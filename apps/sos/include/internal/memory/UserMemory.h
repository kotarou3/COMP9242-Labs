#pragma once

#include <utility>
#include <vector>

#include "internal/memory/PageDirectory.h"
#include "internal/process/Thread.h"

namespace memory {

class UserMemory {
    public:
        UserMemory(process::Process& process, vaddr_t address);

        void read(uint8_t* to, size_t bytes, bool bypassAttributes = false);
        void write(const uint8_t* from, size_t bytes, bool bypassAttributes = false);

        template <typename T>
        T get(bool bypassAttributes = false) {
            T result;
            read(reinterpret_cast<uint8_t*>(&result), sizeof(result), bypassAttributes);
            return result;
        }

        template <typename T>
        std::vector<T> get(size_t length, bool bypassAttributes = false) {
            std::vector<T> result(length);
            read(reinterpret_cast<uint8_t*>(result.data()), length * sizeof(T), bypassAttributes);
            return result;
        }

        template <typename T>
        void set(const T& value, bool bypassAttributes = false) {
            write(reinterpret_cast<const uint8_t*>(&value), sizeof(T), bypassAttributes);
        }

        template <typename T>
        std::pair<T*, ScopedMapping> mapIn(size_t length, Attributes attributes, bool bypassAttributes = false) {
            auto map = _mapIn(length * sizeof(T), attributes, bypassAttributes);
            return std::make_pair(reinterpret_cast<T*>(map.first), std::move(map.second));
        }

    private:
        std::pair<uint8_t*, ScopedMapping> _mapIn(size_t bytes, Attributes attributes, bool bypassAttributes);

        process::Process& _process;
        vaddr_t _address;
};

}
