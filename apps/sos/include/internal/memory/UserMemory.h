#pragma once

#include <vector>

#include "internal/memory/PageDirectory.h"
#include "internal/process/Thread.h"

namespace memory {

class UserMemory {
    public:
        UserMemory(process::Process& process, vaddr_t address);

        std::vector<uint8_t> read(size_t bytes, bool bypassAttributes = false);
        void read(uint8_t* to, size_t bytes, bool bypassAttributes = false);
        void write(const uint8_t* from, size_t bytes, bool bypassAttributes = false);

        template <typename T>
        T get(bool bypassAttributes = false) {
            T result;
            read(reinterpret_cast<uint8_t*>(&result), sizeof(result), bypassAttributes);
            return result;
        }

        template <typename T>
        void set(const T& value, bool bypassAttributes = false) {
            write(reinterpret_cast<const uint8_t*>(&value), sizeof(T), bypassAttributes);
        }

    private:
        ScopedMapping _mapIn(size_t bytes, Attributes attributes, bool bypassAttributes);

        process::Process& _process;
        vaddr_t _address;
};

}
