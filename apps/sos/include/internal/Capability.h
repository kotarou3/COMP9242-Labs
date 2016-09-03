#pragma once

#include <string>
#include <system_error>
#include <utility>

#include <assert.h>

extern "C" {
    #include <cspace/cspace.h>
    #include <sel4/types.h>

    #include "internal/ut_manager/ut.h"
}

// std::unique_ptr-like class for caps

template <seL4_Word Type, seL4_Word SizeBits>
class Capability {
    public:
        Capability() {
            _memory = ut_alloc(SizeBits);
            if (_memory == 0)
                throw std::system_error(ENOMEM, std::system_category(), "Failed to allocate seL4 memory");

            int err = cspace_ut_retype_addr(_memory, Type, SizeBits, cur_cspace, &_cap);
            if (err != seL4_NoError) {
                ut_free(_memory, SizeBits);
                throw std::system_error(ENOMEM, std::system_category(), "Failed to retype seL4 memory: " + std::to_string(err));
            }

            // cspace should never return a cap equal to 0
            assert(_cap != 0);
        }
        Capability(seL4_Word memory, seL4_CPtr cap): _memory(memory), _cap(cap) {}

        ~Capability() {
            if (_cap != 0)
                assert(cspace_delete_cap(cur_cspace, _cap) == CSPACE_NOERROR);
            if (_memory != 0)
                ut_free(_memory, SizeBits);
        }

        Capability(const Capability&) = delete;
        Capability& operator=(const Capability&) = delete;

        Capability(Capability&& other) noexcept {*this = std::move(other);}
        Capability& operator=(Capability&& other) noexcept {
            _memory = std::move(other._memory);
            _cap = std::move(other._cap);
            other._memory = 0;
            other._cap = 0;

            return *this;
        }

        explicit operator bool() const noexcept {return _cap;}

        seL4_Word getMemory() const noexcept {return _memory;}
        seL4_CPtr get() const noexcept {return _cap;}

        std::pair<seL4_Word, seL4_CPtr> release() noexcept {
            auto result = std::make_pair(_memory, _cap);
            _memory = 0;
            _cap = 0;
            return result;
        }

    private:
        seL4_Word _memory;
        seL4_CPtr _cap;
};
