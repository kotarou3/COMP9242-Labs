#pragma once

#include <memory>
#include <string>

#include <fcntl.h>

#define syscall(...) _syscall(__VA_ARGS__)
#include <boost/thread/future.hpp>
#undef syscall

#include "internal/memory/UserMemory.h"

namespace fs {

constexpr const off64_t CURRENT_OFFSET = -1;

struct IoVector {
    memory::UserMemory buffer;
    size_t length;
};

class File {
    public:
        virtual ~File() = default;

        virtual boost::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset);
        virtual boost::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset);

        virtual boost::future<int> ioctl(size_t request, size_t arg);
};

class FileSystem {
    public:
        virtual ~FileSystem() = default;

        virtual boost::future<std::shared_ptr<File>> open(const std::string& pathname) = 0;
};

}
