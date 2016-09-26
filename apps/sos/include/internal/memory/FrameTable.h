#pragma once

#include <limits.h>

extern "C" {
    #include <sel4/types.h>
}

#include "internal/async.h"

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
class Swap;

namespace FrameTable {
    class Frame {
        public:
            Frame(const Frame&) = delete;
            Frame(Frame&&) = delete;
            Frame& operator=(const Frame&) = delete;
            Frame& operator=(Frame&&) = delete;

            void insert(Page& page) noexcept;
            void erase(Page& page) noexcept;

            void disableReference() noexcept;
            void updateStatus() noexcept;

            paddr_t getAddress() const;

        private:
            Frame():
                _pages(nullptr),
                _isLocked(false),
                _isReferenced(false)
            {}

            Page* _pages;

            bool _isLocked:1;
            bool _isReferenced:1;

            friend class ::memory::Page;
            friend class ::memory::Swap;
            friend void init(paddr_t start, paddr_t end);
            friend async::future<Page> alloc();
    };

    void init(paddr_t start, paddr_t end);

    async::future<Page> alloc();
    Page alloc(paddr_t address);
}

bool isReady() noexcept;

}
