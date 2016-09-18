#include <string>
#include <system_error>
#include <vector>
#include <assert.h>

#include "internal/memory/FrameTable.h"
#include "internal/memory/PageDirectory.h"
#include "internal/process/Thread.h"
#include "internal/memory/Swap.h"

extern "C" {
    #include <cspace/cspace.h>
    #include "internal/ut_manager/ut.h"
}

namespace memory {
namespace FrameTable {

namespace {
    Frame* _table;
    paddr_t _start, _end;
    size_t _npages, _frameCount;

    inline Frame& _getFrame(paddr_t address) {
        assert(_start <= address && address < _end);
        return _table[(address - _start) / PAGE_SIZE];
    }
}

inline paddr_t Frame::getAddress() const {
    return _start + ((this - _table) * PAGE_SIZE);
}

void init(paddr_t start, paddr_t end) {
    _start = start;
    _end = end;

    _frameCount = numPages(end - start);
    size_t frameTableSize = _frameCount * sizeof(Frame);
    _npages = numPages(frameTableSize);

    // Allocate a place in virtual memory to place the frame table
    auto tableMap = process::getSosProcess().maps.insert(
        0, _npages,
        Attributes{.read = true, .write = true},
        Mapping::Flags{.shared = false}
    );
    _table = reinterpret_cast<Frame*>(tableMap.getAddress());

    // Allocate the frame table
    std::vector<std::pair<paddr_t, vaddr_t>> frameTableAddresses;
    frameTableAddresses.reserve(_npages);
    for (size_t p = 0; p < _npages; ++p) {
        paddr_t phys = ut_alloc(seL4_PageBits);
        vaddr_t virt = tableMap.getAddress() + (p * PAGE_SIZE);

        process::getSosProcess().pageDirectory.map(
            Page(phys), virt,
            Attributes{.read = true, .write = true}
        );

        frameTableAddresses.push_back(std::make_pair(phys, virt));
    }

    // Construct the frames
    for (size_t p = 0; p < _frameCount; ++p)
        new(&_table[p]) Frame;

    // Connect the frame table frames to the pages
    for (const auto& pair : frameTableAddresses) {
        Frame& frame = _getFrame(pair.first);
        frame.pages = const_cast<Page*>(&process::getSosProcess().pageDirectory.lookup(pair.second)->_page);
        frame.pages->_frame = &frame;
    }

    // Check the allocations were correct
    assert(_table[0].getAddress() == start);
    assert(_table[_frameCount - 1].getAddress() == end - PAGE_SIZE);

    tableMap.release();
}

void Frame::disableReference() {
    referenced = false;
    // unmap all the pages associated with said frame
    Page* current = pages;
    while (current != nullptr) {
        assert(seL4_ARM_Page_Unmap(current->getCap()) == seL4_NoError);
        current->referenced = false;
        current = current->_next;
    }
}

boost::future<Page> alloc(bool locked) {
    paddr_t address = ut_alloc(seL4_PageBits);
    if (!address) {
        static unsigned int clock = -1;
        while (_table[clock = (clock + 1) % _frameCount].referenced || _table[clock].locked || _table[clock].pages == nullptr)
            if (!_table[clock].locked && _table[clock].pages != nullptr)
                _table[clock].disableReference();

        Page* page = _table[clock].pages;
        return memory::Swap::get().swapout(*page).then(fs::asyncExecutor, [&] (auto id) {
            while (page != nullptr) {
                page->_frame = reinterpret_cast<Frame*>(id.get());
                page->_paged = true;
                page = page->_next;
            }
            _table[clock].locked = locked;
            return Page(_table[clock]);
        });
    }
    _getFrame(address).locked = locked;
    boost::promise<Page> promise;
    promise.set_value(Page(_getFrame(address)));
    return promise.get_future();
}

Page alloc(paddr_t address) {
    return Page(address);
}

}

Page::Page(FrameTable::Frame& frame):
    Page(frame.getAddress())
{
    _frame = &frame;

    assert(_frame->pages == nullptr);
    _frame->pages = this;
}

Page::Page(paddr_t address):
    _frame(nullptr),
    _prev(nullptr),
    _next(nullptr)
{
    int err = cspace_ut_retype_addr(
        address,
        seL4_ARM_SmallPageObject, seL4_PageBits,
        cur_cspace, &_cap
    );
    if (err != seL4_NoError)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to retype to a seL4 page: " + std::to_string(err));

    // The CSpace library should never return a 0 cap
    assert(_cap != 0);
}

Page::~Page() {
    if (_cap) {
        assert(cspace_delete_cap(cur_cspace, _cap) == CSPACE_NOERROR);

        if (_frame) {
            if (_frame->pages == this) {
                assert(!_prev);
                _frame->pages = _next;
            } else {
                assert(_prev);
            }

            if (!_frame->pages)
                // We were the last copy, so free the frame
                ut_free(_frame->getAddress(), seL4_PageBits);
        }

        if (_prev)
            _prev->_next = _next;
        if (_next)
            _next->_prev = _prev;
    }
}

Page::Page(const Page& other):
    _frame(other._frame),
    _prev(const_cast<Page*>(&other)),
    _next(other._next)
{
    _cap = cspace_copy_cap(cur_cspace, cur_cspace, other._cap, seL4_AllRights);
    if (_cap == CSPACE_NULL)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to copy page cap");
    assert(_cap != 0);

    _prev->_next = this;
    if (_next)
        _next->_prev = this;
}

Page::Page(Page&& other) noexcept {
    *this = std::move(other);
}

Page& Page::operator=(Page&& other) noexcept {
    _cap = std::move(other._cap);
    _frame = std::move(other._frame);
    _prev = std::move(other._prev);
    _next = std::move(other._next);

    other._cap = 0;
    other._frame = nullptr;
    other._prev = nullptr;
    other._next = nullptr;

    if (_frame) {
        if (_frame->pages == &other) {
            assert(!_prev);
            _frame->pages = this;
        } else {
            assert(_prev);
        }
    }

    if (_prev) {
        assert(_prev->_next == &other);
        _prev->_next = this;
    }
    if (_next) {
        assert(_next->_prev == &other);
        _next->_prev = this;
    }

    return *this;
}

}
