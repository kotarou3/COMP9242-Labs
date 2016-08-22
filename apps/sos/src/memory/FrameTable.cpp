#include <string>
#include <vector>
#include <assert.h>

#include "internal/memory/FrameTable.h"
#include "internal/memory/PageDirectory.h"

extern "C" {
    #include <cspace/cspace.h>
    #include "internal/ut_manager/ut.h"
}

namespace memory {
namespace FrameTable {

struct Frame {
    Page* pages;

    inline paddr_t getAddress() const;
};

namespace {
    Frame* _table;
    paddr_t _start, _end;

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

    size_t frameCount = numPages(end - start);
    size_t frameTableSize = frameCount * sizeof(Frame);
    size_t frameTablePages = numPages(frameTableSize);

    // Pick some place in virtual memory to place the frame table
    // TODO: Automatically choose a location in m3
    _table = reinterpret_cast<Frame*>(0xc0000000);

    // Allocate the frame table
    std::vector<std::pair<paddr_t, vaddr_t>> frameTableAddresses;
    frameTableAddresses.reserve(frameTablePages);
    for (size_t p = 0; p < frameTablePages; ++p) {
        paddr_t phys = ut_alloc(seL4_PageBits);
        vaddr_t virt = reinterpret_cast<vaddr_t>(_table) + (p * PAGE_SIZE);

        sosPageDirectory.map(
            Page(phys), virt,
            Attributes{.read = true, .write = true}
        );

        frameTableAddresses.push_back(std::make_pair(phys, virt));
    }

    // Construct the frames
    for (size_t p = frameTablePages; p < frameCount; ++p)
        new(&_table[p]) Frame;

    // Connect the frame table frames to the pages
    for (const auto& pair : frameTableAddresses) {
        Frame& frame = _getFrame(pair.first);
        frame.pages = const_cast<Page*>(&sosPageDirectory.lookup(pair.second)._page);
        frame.pages->_frame = &frame;
    }

    // Check the allocations were correct
    assert(_table[0].getAddress() == start);
    assert(_table[frameCount - 1].getAddress() == end - PAGE_SIZE);
}

Page alloc() {
    paddr_t address = ut_alloc(seL4_PageBits);
    if (!address)
        throw std::runtime_error("Out of physical frames\n");

    return Page(_getFrame(address));
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
    _frame(nullptr)
{
    int err = cspace_ut_retype_addr(
        address,
        seL4_ARM_SmallPageObject, seL4_PageBits,
        cur_cspace, &_cap
    );
    if (err != seL4_NoError)
        throw std::runtime_error("Failed to retype to a seL4 page: " + std::to_string(err));

    // The CSpace library should never return a 0 cap
    assert(_cap != 0);
}

Page::~Page() {
    if (_cap) {
        assert(cspace_delete_cap(cur_cspace, _cap) == CSPACE_NOERROR);

        if (_frame) {
            assert(_frame->pages == this);
            _frame->pages = nullptr;

            ut_free(_frame->getAddress(), seL4_PageBits);
        }
    }
}

Page::Page(Page&& other) noexcept {
    *this = std::move(other);
}

Page& Page::operator=(Page&& other) noexcept {
    _cap = std::move(other._cap);
    _frame = std::move(other._frame);
    other._cap = 0;
    other._frame = nullptr;

    if (_frame) {
        assert(_frame->pages == &other);
        _frame->pages = this;
    }

    return *this;
}

}
