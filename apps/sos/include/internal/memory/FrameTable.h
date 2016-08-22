#pragma once

#include <limits.h>

extern "C" {
    #include <sel4/types.h>
}

namespace memory {

using paddr_t = size_t;

static_assert(PAGE_SIZE == (1 << seL4_PageBits), "Incorrect value of PAGE_SIZE");

constexpr size_t pageAlign(size_t value) {
    return value & -PAGE_SIZE;
}
constexpr size_t pageOffset(size_t value) {
    return value & ~-PAGE_SIZE;
}
constexpr size_t numPages(size_t bytes) {
    return (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
}

class Page;

namespace FrameTable {
    void init(paddr_t start, paddr_t end);

    Page alloc();

    class Frame;
}

class Page {
    public:
        ~Page();

        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;

        Page(Page&& other) noexcept;
        Page& operator=(Page&& other) noexcept;

        seL4_ARM_Page getCap() const noexcept {return _cap;};

    private:
        Page(FrameTable::Frame& frame);
        Page(FrameTable::Frame& frame, paddr_t address);
        friend void FrameTable::init(paddr_t start, paddr_t end);
        friend Page FrameTable::alloc();

        seL4_ARM_Page _cap;
        FrameTable::Frame* _frame;
};

}
