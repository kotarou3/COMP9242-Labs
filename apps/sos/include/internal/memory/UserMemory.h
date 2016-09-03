#pragma once

#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "internal/memory/PageDirectory.h"
#include "internal/process/Thread.h"

namespace memory {

class UserMemory {
    public:
        UserMemory(process::Process& process, vaddr_t address);

        std::string readString(bool bypassAttributes = false);

        template <typename It, typename = std::enable_if_t<std::is_pod<typename std::iterator_traits<It>::value_type>::value>>
        void read(It begin, It end, bool bypassAttributes = false) {
            size_t length = end - begin;
            auto map = mapIn<typename std::iterator_traits<It>::value_type>(
                length, Attributes{.read = true}, bypassAttributes
            );
            std::copy(map.first, map.first + length, begin);
        }

        template <typename It, typename = std::enable_if_t<std::is_pod<typename std::iterator_traits<It>::value_type>::value>>
        void write(It begin, It end, bool bypassAttributes = false) {
            size_t length = end - begin;
            auto map = mapIn<typename std::iterator_traits<It>::value_type>(
                length, Attributes{.read = false, .write = true}, bypassAttributes
            );
            std::copy(begin, end, map.first);
        }

        template <typename T>
        inline void write(T* item) {
            write(item, item + 1);
        }

        template <typename T>
        T get(bool bypassAttributes = false) {
            T result;
            read(&result, &result + 1, bypassAttributes);
            return result;
        }

        template <typename T>
        std::vector<T> get(size_t length, bool bypassAttributes = false) {
            std::vector<T> result(length);
            read(result.begin(), result.end(), bypassAttributes);
            return result;
        }

        template <typename T>
        void set(const T& value, bool bypassAttributes = false) {
            write(&value, &value + 1, bypassAttributes);
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
