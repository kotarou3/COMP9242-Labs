#pragma once

#include <limits.h>

extern "C" {
    #include <sel4/types.h>
}

namespace process {class Process;}
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
        bool pinned = true;
        // true if any of the pages associated with it have the reference bit set
        bool reference = true;

        inline paddr_t getAddress() const;
    };
    void init(paddr_t start, paddr_t end);

    void disableReference(Frame&);
    void enableReference(process::Process&, const MappedPage&);

    Page alloc(bool pinned = true);
    Page alloc(paddr_t address);

    class Frame;
}

class Page {
    public:
        ~Page();

        Page(Page&& other) noexcept;
        Page& operator=(Page&& other) noexcept;

        Page copy() const {return *this;};

        seL4_ARM_Page getCap() const noexcept {return _cap;}

        explicit operator bool() const noexcept {return _cap;}
        mutable bool reference = true;

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

        friend void FrameTable::disableReference(FrameTable::Frame &);
        friend void FrameTable::enableReference(process::Process &, const MappedPage &);
        friend void FrameTable::init(paddr_t start, paddr_t end);
        friend Page FrameTable::alloc(bool pinned);
        friend Page FrameTable::alloc(paddr_t address);
};

}
