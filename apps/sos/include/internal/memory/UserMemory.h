#pragma once

#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "internal/async.h"
#include "internal/memory/PageDirectory.h"
#include "internal/process/Thread.h"

namespace memory {

class UserMemory {
    public:
        UserMemory(std::weak_ptr<process::Process> process, vaddr_t address);

        async::future<std::string> readString(bool bypassAttributes = false);

        template <typename It, typename = std::enable_if_t<std::is_pod<typename std::iterator_traits<It>::value_type>::value>>
        async::future<void> read(It begin, It end, bool bypassAttributes = false) {
            size_t length = end - begin;

            return mapIn<typename std::iterator_traits<It>::value_type>(
                length, Attributes{.read = true}, bypassAttributes
            ).then([begin, length](auto map) {
                auto map_ = std::move(map.get());
                std::copy(map_.first, map_.first + length, begin);
            });
        }

        template <typename It, typename = std::enable_if_t<std::is_pod<typename std::iterator_traits<It>::value_type>::value>>
        async::future<void> write(It begin, It end, bool bypassAttributes = false) {
            size_t length = end - begin;

            return mapIn<typename std::iterator_traits<It>::value_type>(
                length, Attributes{.read = false, .write = true}, bypassAttributes
            ).then([begin, end](auto map) {
                auto map_ = std::move(map.get());
                std::copy(begin, end, map_.first);
            });
        }

        template <typename T>
        async::future<T> get(bool bypassAttributes = false) {
            auto out = std::make_shared<T>();
            return read(out.get(), out.get() + 1, bypassAttributes)
                .then([out](async::future<void> result) {
                    result.get();
                    return std::move(*out);
                });
        }

        template <typename T>
        async::future<std::vector<T>> get(size_t length, bool bypassAttributes = false) {
            auto out = std::make_shared<std::vector<T>>(length);
            return read(out->begin(), out->end(), bypassAttributes)
                .then([out](async::future<void> result) {
                    result.get();
                    return std::move(*out);
                });
        }

        template <typename T>
        async::future<void> set(const T& value, bool bypassAttributes = false) {
            return write(&value, &value + 1, bypassAttributes);
        }

        template <typename T>
        async::future<std::pair<T*, ScopedMapping>> mapIn(size_t length, Attributes attributes, bool bypassAttributes = false) {
            return _mapIn(length * sizeof(T), attributes, bypassAttributes)
                .then([](auto map) {
                    auto map_ = std::move(map.get());
                    return std::make_pair(reinterpret_cast<T*>(map_.first), std::move(map_.second));
                });
        }

    private:
        async::future<std::pair<uint8_t*, ScopedMapping>> _mapIn(size_t bytes, Attributes attributes, bool bypassAttributes);

        std::weak_ptr<process::Process> _process;
        vaddr_t _address;
};

}
