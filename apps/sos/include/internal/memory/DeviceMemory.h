#pragma once

#include "internal/memory/Mappings.h"
#include "internal/process/Thread.h"

extern "C" {
    #include <sel4/types.h>
}

namespace memory {

template <typename T>
class DeviceMemory {
    public:
        DeviceMemory(paddr_t address):
            _map(process::getSosProcess().maps.insert(
                0, numPages(sizeof(T)),
                Attributes{
                    .read = true,
                    .write = true,
                    .execute = false,
                    .locked = true,
                    .notCacheable = true
                },
                Mapping::Flags{.shared = false}
            ))
        {
            for (size_t offset = 0; offset < sizeof(T); offset += PAGE_SIZE) {
                process::getSosProcess().pageDirectory.map(
                    FrameTable::alloc(address + offset), _map.getAddress() + offset,
                    Attributes{
                        .read = true,
                        .write = true,
                        .execute = false,
                        .locked = true,
                        .notCacheable = true
                    }
                );
            }
        }

        const volatile T& operator*() const noexcept {
            return *reinterpret_cast<const volatile T*>(_map.getAddress());
        }
        volatile T& operator*() noexcept {
            return *reinterpret_cast<volatile T*>(_map.getAddress());
        }

        const volatile T* operator->() const noexcept {
            return &**this;
        }
        volatile T* operator->() noexcept {
            return &**this;
        }

    private:
        ScopedMapping _map;
};

}
