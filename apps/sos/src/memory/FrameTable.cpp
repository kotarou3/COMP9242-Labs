#include <stdexcept>
#include <vector>
#include <assert.h>

extern "C" {
    #include "internal/ut_manager/ut.h"
}

#include "internal/memory/FrameTable.h"
#include "internal/memory/Page.h"
#include "internal/memory/PageDirectory.h"
#include "internal/memory/Swap.h"
#include "internal/process/Thread.h"

namespace memory {

namespace {
    bool _isReady;
}

namespace FrameTable {

namespace {
    Frame* _table;
    paddr_t _start, _end;
    size_t _frameTablePages, _frameCount;

    inline Frame& _getFrame(paddr_t address) {
        assert(_start <= address && address < _end);
        return _table[(address - _start) / PAGE_SIZE];
    }
}

void Frame::insert(Page& page) noexcept {
    assert(page._status == Page::Status::UNREFERENCED);
    assert(!page._resident.frame);
    assert(!page._prev && !page._next);

    page._resident.frame = this;

    if (!_pages) {
        _pages = &page;
    } else {
        assert(!_pages->_prev);
        switch (_pages->_status) {
            case Page::Status::INVALID:
            case Page::Status::SWAPPED:
                assert(false);

            default:
                assert(_pages->_resident.frame == this);
        }

        page._next = _pages;
        _pages->_prev = &page;
        _pages = &page;
    }
}

void Frame::erase(Page& page) noexcept {
    assert(page._status == Page::Status::UNREFERENCED);
    assert(_pages && page._resident.frame == this);

    if (_pages == &page) {
        assert(!page._prev);
        _pages = page._next;
    } else {
        assert(page._prev);
    }

    if (!_pages)
        // We were the last copy, so free the frame
        ut_free(getAddress(), seL4_PageBits);

    if (page._prev)
        page._prev->_next = page._next;
    if (page._next)
        page._next->_prev = page._prev;
}

void Frame::disableReference() noexcept {
    assert(_isReferenced == true);

    // Unmap all the pages associated with said frame
    for (Page* page = _pages; page != nullptr; page = page->_next) {
        assert(page->_status == Page::Status::REFERENCED);
        assert(page->_resident.frame == this);

        assert(seL4_ARM_Page_Unmap(page->getCap()) == seL4_NoError);
        page->_status = Page::Status::UNREFERENCED;
    }

    _isReferenced = false;
}

void Frame::updateStatus() noexcept {
    _isLocked = false;
    _isReferenced = false;
    for (Page* page = _pages; page != nullptr; page = page->_next) {
        switch (_pages->_status) {
            case Page::Status::INVALID:
            case Page::Status::SWAPPED:
                assert(false);

            default:
                assert(page->_resident.frame == this);
        }

        _isLocked |= page->_status == Page::Status::LOCKED;
        _isReferenced |= _isLocked || page->_status == Page::Status::REFERENCED;
    }
}

paddr_t Frame::getAddress() const {
    return _start + ((this - _table) * PAGE_SIZE);
}

void init(paddr_t start, paddr_t end) {
    _start = start;
    _end = end;

    _frameCount = numPages(end - start);
    size_t frameTableSize = _frameCount * sizeof(Frame);
    _frameTablePages = numPages(frameTableSize);

    // Allocate a place in virtual memory to place the frame table
    Attributes attributes = {
        .read = true,
        .write = true,
        .execute = false,
        .locked = true
    };
    auto tableMap = process::getSosProcess().maps.insert(
        0, _frameTablePages,
        attributes,
        Mapping::Flags{.shared = false}
    );
    _table = reinterpret_cast<Frame*>(tableMap.getAddress());

    // Allocate the frame table
    std::vector<std::pair<paddr_t, vaddr_t>> frameTableAddresses;
    frameTableAddresses.reserve(_frameTablePages);
    for (size_t p = 0; p < _frameTablePages; ++p) {
        paddr_t phys = ut_alloc(seL4_PageBits);
        vaddr_t virt = tableMap.getAddress() + (p * PAGE_SIZE);

        process::getSosProcess().pageDirectory.map(Page(phys), virt, attributes);

        frameTableAddresses.push_back(std::make_pair(phys, virt));
    }

    // Construct the frames
    for (size_t p = 0; p < _frameCount; ++p)
        new(&_table[p]) Frame;

    // Connect the frame table frames to the pages
    for (const auto& pair : frameTableAddresses) {
        Page& page = const_cast<Page&>(process::getSosProcess().pageDirectory.lookup(pair.second)->_page);
        Frame& frame = _getFrame(pair.first);

        assert(page._status == Page::Status::LOCKED);
        page._status = Page::Status::UNREFERENCED;
        frame.insert(page);
        page._status = Page::Status::LOCKED;
        frame.updateStatus();
    }

    // Check the allocations were correct
    assert(_table[0].getAddress() == start);
    assert(_table[_frameCount - 1].getAddress() == end - PAGE_SIZE);

    tableMap.release();

    _isReady = true;
}

async::future<Page> alloc() {
    paddr_t address = ut_alloc(seL4_PageBits);
    if (!address) {
        static size_t clock;

        Frame* toSwap = nullptr;

        size_t n = 1;
        for (; n < _frameCount * 2; ++n) {
            size_t f = (clock + n) % _frameCount;
            if (!_table[f]._pages || _table[f]._isLocked)
                continue;

            if (_table[f]._isReferenced) {
                _table[f].disableReference();
            } else {
                toSwap = &_table[f];
                break;
            }
        }
        clock = (clock + n) % _frameCount;

        if (!toSwap)
            throw std::bad_alloc();

        return memory::Swap::get().swapOut(*toSwap)
            .then([](async::future<void> result) {
                result.get();
                return alloc();
            });
    }

    return async::make_ready_future(Page(_getFrame(address)));
}

Page alloc(paddr_t address) {
    return Page(address);
}

}

bool isReady() noexcept {
    return _isReady;
}

}
