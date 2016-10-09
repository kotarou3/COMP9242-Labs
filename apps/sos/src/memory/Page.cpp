#include <string>
#include <system_error>

extern "C" {
    #include <cspace/cspace.h>
    #include "internal/ut_manager/ut.h"
}

#include "internal/memory/FrameTable.h"
#include "internal/memory/Page.h"
#include "internal/memory/Swap.h"

namespace memory {

Page::Page():
    _status(Status::INVALID),
    _resident({.cap = 0, .frame = nullptr}),
    _prev(nullptr),
    _next(nullptr)
{}

Page::Page(FrameTable::Frame& frame):
    Page(frame.getAddress())
{
    assert(frame._pages == nullptr);
    frame.insert(*this);
}

Page::Page(paddr_t address):
    Page()
{
    int err = cspace_ut_retype_addr(
        address,
        seL4_ARM_SmallPageObject, seL4_PageBits,
        cur_cspace, &_resident.cap
    );
    if (err != seL4_NoError)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to retype to a seL4 page: " + std::to_string(err));

    // The CSpace library should never return a 0 cap
    assert(_resident.cap != 0);

    _status = Status::UNMAPPED;
}

Page::~Page() {
    switch (_status) {
        case Status::INVALID:
            break;

        case Status::LOCKED:
        case Status::REFERENCED:
        case Status::UNREFERENCED:
            // This should never happen, since MappedPage will remove these
            // statuses when it's destroyed
            assert(false);

        case Status::UNMAPPED:
            assert(cspace_delete_cap(cur_cspace, _resident.cap) == CSPACE_NOERROR);

            if (_resident.frame)
                _resident.frame->erase(*this);
            else
                assert(!_prev && !_next);

            break;

        case Status::SWAPPED:
            Swap::get().erase(*this);
            break;
    }
}

Page::Page(const Page& other):
    _status(Status::INVALID),
    _resident({.cap = 0, .frame = nullptr}),
    _prev(nullptr),
    _next(nullptr)
{
    switch (other._status) {
        case Status::INVALID:
            return;

        case Status::LOCKED:
        case Status::REFERENCED:
        case Status::UNREFERENCED:
        case Status::UNMAPPED:
            _resident.cap = cspace_copy_cap(cur_cspace, cur_cspace, other._resident.cap, seL4_AllRights);
            if (_resident.cap == CSPACE_NULL)
                throw std::system_error(ENOMEM, std::system_category(), "Failed to copy page cap");
            assert(_resident.cap != 0);

            _status = Status::UNMAPPED;
            assert(other._resident.frame); // Non-frame pages cannot be copied
            other._resident.frame->insert(*this);
            break;

        case Status::SWAPPED:
            Swap::get().copy(other, *this);
            break;
    }
}

Page::Page(Page&& other) noexcept:
    Page()
{
    *this = std::move(other);
}

Page& Page::operator=(Page&& other) noexcept {
    // XXX: Only supported for moving to an invalid Page
    assert(_status == Status::INVALID);

    _status = std::move(other._status);
    _prev = std::move(other._prev);
    _next = std::move(other._next);

    other._status = Status::INVALID;
    other._prev = nullptr;
    other._next = nullptr;

    if (_prev) {
        assert(_prev->_next == &other);
        _prev->_next = this;
    }
    if (_next) {
        assert(_next->_prev == &other);
        _next->_prev = this;
    }

    switch (_status) {
        case Status::INVALID:
            break;

        case Status::LOCKED:
        case Status::REFERENCED:
        case Status::UNREFERENCED:
        case Status::UNMAPPED:
            _resident = std::move(other._resident);

            if (_resident.frame) {
                if (_resident.frame->_pages == &other) {
                    assert(!_prev);
                    _resident.frame->_pages = this;
                } else {
                    assert(_prev);
                }
            }
            break;

        case Status::SWAPPED:
            _swapId = std::move(other._swapId);
            break;
    }

    return *this;
}

}
