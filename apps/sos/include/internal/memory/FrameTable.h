#pragma once

#include <limits.h>

#define syscall(...) _syscall(__VA_ARGS__)
#include <boost/thread/future.hpp>
#undef syscall

extern "C" {
    #include <sel4/types.h>
}

namespace memory {

using paddr_t = size_t;

constexpr size_t PAGE_TABLE_SIZE = ((1 << (seL4_PageTableBits + seL4_PageBits)) / sizeof(seL4_Word));
static_assert(PAGE_SIZE == (1 << seL4_PageBits), "Incorrect value of PAGE_SIZE");

constexpr size_t pageTableAlign(size_t value) {
    return value & -PAGE_TABLE_SIZE;
}
constexpr size_t pageTableOffset(size_t value) {
    return value & ~-PAGE_TABLE_SIZE;
}

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
    struct Frame {
        Frame():
            pages(nullptr),
            isLocked(false),
            isReferenced(false)
        {}

        Page* pages;

        bool isLocked:1;
        bool isReferenced:1;

        void disableReference() noexcept;

        paddr_t getAddress() const;
    };
    void init(paddr_t start, paddr_t end);

    boost::future<Page> alloc(bool isLocked = false);
    Page alloc(paddr_t address);

    class Frame;
}

}
