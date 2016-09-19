#pragma once

#include <limits.h>

#define syscall(...) _syscall(__VA_ARGS__)
#include <boost/thread/future.hpp>
#undef syscall

extern "C" {
    #include <sel4/types.h>
}

class MappedPage;

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
class MappedPage;

namespace FrameTable {
    struct Frame {
        Page* pages = nullptr;
        bool locked = true;
        // true if any of the pages associated with it have the reference bit set
        bool referenced = true;
        void disableReference();

        inline paddr_t getAddress() const;
    };
    void init(paddr_t start, paddr_t end);

    boost::future<Page> alloc(bool locked = true);
    Page alloc(paddr_t locked);

    class Frame;
}

class Page {
    public:
        ~Page();

        Page(Page&& other) noexcept;
        Page& operator=(Page&& other) noexcept;

        Page copy() const {return *this;};
        void swapOut(unsigned int id);

        seL4_ARM_Page getCap() const noexcept {return _cap;}

        explicit operator bool() const noexcept {return _cap;}
        mutable bool referenced = true;

private:
        Page(FrameTable::Frame& frame);
        Page(paddr_t address);

        Page(const Page& other);
        Page& operator=(const Page&) = delete;
        bool _paged = false;

        seL4_ARM_Page _cap;
        FrameTable::Frame* _frame;

        Page* _prev;
        mutable Page* _next;

        friend void FrameTable::init(paddr_t start, paddr_t end);
        friend boost::future<Page> FrameTable::alloc(bool locked);
        friend Page FrameTable::alloc(paddr_t locked);
        friend void FrameTable::Frame::disableReference();
        friend class MappedPage;
};

}
