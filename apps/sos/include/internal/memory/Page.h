#pragma once

extern "C" {
    #include <sel4/types.h>
}

#include "internal/async.h"

namespace memory {

using SwapId = size_t;

class Page {
    public:
        Page();
        ~Page();

        Page(Page&& other) noexcept;
        Page& operator=(Page&& other) noexcept;

        Page copy() const {return *this;};

        enum class Status {
            INVALID,        // Happens when default constructed or moved away
            LOCKED,         // Mapped and locked in memory
            REFERENCED,     // Mapped in memory
            UNREFERENCED,   // Unmapped in memory
            UNMAPPED,       // Unmapped in memory, but not swappable. Only occurs on newly created Pages
            SWAPPED         // Swapped out
        };

        Status getStatus() const noexcept {return _status;}
        seL4_ARM_Page getCap() const noexcept {
            switch (_status) {
                case Status::LOCKED:
                case Status::REFERENCED:
                case Status::UNREFERENCED:
                case Status::UNMAPPED:
                    break;

                default:
                    assert(false);
            }
            return _resident.cap;
        }

        explicit operator bool() const noexcept {return _status != Status::INVALID;}

    private:
        Page(FrameTable::Frame& frame);
        Page(paddr_t address);

        Page(const Page& other);
        Page& operator=(const Page&) = delete;

        Status _status;

        union {
            struct {
                seL4_ARM_Page cap;
                FrameTable::Frame* frame;
            } _resident;

            SwapId _swapId;
        };

        Page* _prev;
        mutable Page* _next;

        friend void FrameTable::init(paddr_t start, paddr_t end);
        friend async::future<Page> FrameTable::alloc();
        friend Page FrameTable::alloc(paddr_t address);

        friend class FrameTable::Frame;
        friend class MappedPage;
        friend class Swap;
};

}
