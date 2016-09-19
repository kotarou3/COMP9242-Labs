#include <stdexcept>
#include <vector>
#include <assert.h>

#include "internal/memory/FrameTable.h"
#include "internal/memory/Page.h"
#include "internal/memory/PageDirectory.h"
#include "internal/memory/Swap.h"
#include "internal/process/Thread.h"

extern "C" {
    #include "internal/ut_manager/ut.h"
}

namespace memory {
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

void Frame::disableReference() {
    assert(isReferenced == false);

    // Unmap all the pages associated with said frame
    for (Page* page = pages; page != nullptr; page = page->_next) {
        assert(page->_status == Page::Status::REFERENCED);
        assert(seL4_ARM_Page_Unmap(page->getCap()) == seL4_NoError);
        page->_status = Page::Status::UNREFERENCED;
    }

    isReferenced = false;
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
    auto tableMap = process::getSosProcess().maps.insert(
        0, _frameTablePages,
        Attributes{.read = true, .write = true},
        Mapping::Flags{.shared = false}
    );
    _table = reinterpret_cast<Frame*>(tableMap.getAddress());

    // Allocate the frame table
    std::vector<std::pair<paddr_t, vaddr_t>> frameTableAddresses;
    frameTableAddresses.reserve(_frameTablePages);
    for (size_t p = 0; p < _frameTablePages; ++p) {
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

boost::future<Page> alloc(bool isLocked) {
    paddr_t address = ut_alloc(seL4_PageBits);
    if (!address) {
        static size_t clock;

        Frame* toSwap[PARALLEL_SWAPS];
        size_t toSwapCount = 0;

        size_t f = (clock + 1) % _frameCount;
        for (; f != clock; f = (f + 1) % _frameCount) {
            if (!_table[f].pages || _table[f].isLocked)
                continue;

            if (_table[f].isReferenced) {
                _table[f].disableReference();
            } else {
                toSwap[toSwapCount++] = &_table[f];
                if (toSwapCount == PARALLEL_SWAPS)
                    break;
            }
        }
        clock = f;

        if (toSwapCount == 0)
            throw std::bad_alloc();

        return memory::Swap::get().swapOut(toSwap, toSwapCount).then(Swap::asyncExecutor, [isLocked] (auto result) {
            result.get();
            return alloc(isLocked);
        });
    }

    _getFrame(address).isLocked = isLocked;
    return boost::make_ready_future(Page(_getFrame(address)));
}

Page alloc(paddr_t address) {
    return Page(address);
}

}
}
