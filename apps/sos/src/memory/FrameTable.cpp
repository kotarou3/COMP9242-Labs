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
    seL4_ARM_Page cap;

    inline paddr_t getAddress() const;
};

namespace {
    Frame* _table;
    paddr_t _start, _end;

    inline Frame& _getFrame(paddr_t address) {
        assert(_start <= address && address < _end);
        return _table[(address - _start) >> seL4_PageBits];
    }
}

inline paddr_t Frame::getAddress() const {
    return _start + ((this - _table) << seL4_PageBits);
}

void init(paddr_t start, paddr_t end) {
    _start = start;
    _end = end;

    size_t frameCount = (end - start) >> seL4_PageBits;
    size_t frameTableSize = frameCount * sizeof(Frame);
    size_t frameTablePages = (frameTableSize + (1 << seL4_PageBits) - 1) >> seL4_PageBits;

    // Pick some place in virtual memory to place the frame table
    // TODO: Automatically choose a location in m3
    _table = reinterpret_cast<Frame*>(0xc0000000);

    // Allocate the frame table
    std::vector<std::pair<paddr_t, Frame>> frameTableFrames;
    frameTableFrames.reserve(frameTablePages);
    for (size_t p = 0; p < frameTablePages; ++p) {
        // Use a temporary frame that we'll later move into the frame table
        Frame frame;
        frame.pages = nullptr;
        paddr_t address = ut_alloc(seL4_PageBits);

        sosPageDirectory.map(
            Page(frame, address), reinterpret_cast<vaddr_t>(_table) + (p << seL4_PageBits),
            Attributes{.read = true, .write = true}
        );

        frameTableFrames.push_back(std::make_pair(address, frame));
    }

    // Construct the frames
    for (size_t p = frameTablePages; p < frameCount; ++p)
        new(&_table[p]) Frame;

    // Move the temporary frames into the frame table
    for (const auto& pair : frameTableFrames) {
        Frame& frame = _getFrame(pair.first) = pair.second;
        frame.pages->_frame = &frame;
    }

    // Check the allocations were correct
    assert(_table[0].getAddress() == start);
    assert(_table[frameCount - 1].getAddress() == end - (1 << seL4_PageBits));
}

Page alloc() {
    paddr_t address = ut_alloc(seL4_PageBits);
    if (!address)
        throw std::runtime_error("Out of physical frames\n");

    return Page(_getFrame(address));
}

}

Page::Page(FrameTable::Frame& frame):
    Page(frame, frame.getAddress())
{}

// Allow specifying physical address directly so FrameTable::init() can use this
// class with a temporary FrameTable::Frame before the frame table is allocated
Page::Page(FrameTable::Frame& frame, paddr_t address):
    _frame(&frame)
{
    assert(_frame->pages == nullptr);

    int err = cspace_ut_retype_addr(
        address,
        seL4_ARM_SmallPageObject, seL4_PageBits,
        cur_cspace, &_frame->cap
    );
    if (err != seL4_NoError)
        throw std::runtime_error("Failed to retype to a seL4 page: " + std::to_string(err));

    _cap = _frame->cap;
    _frame->pages = this;
}

Page::~Page() {
    if (_frame) {
        assert(_frame->pages == this);
        assert(cspace_delete_cap(cur_cspace, _frame->cap) == CSPACE_NOERROR);
        ut_free(_frame->getAddress(), seL4_PageBits);
        _frame->pages = nullptr;
    }
}

Page::Page(Page&& other) noexcept {
    *this = std::move(other);
}

Page& Page::operator=(Page&& other) noexcept {
    assert(other._frame->pages == &other);

    _cap = std::move(other._cap);
    _frame = std::move(other._frame);

    _frame->pages = this;
    other._frame = nullptr;

    return *this;
}

}
