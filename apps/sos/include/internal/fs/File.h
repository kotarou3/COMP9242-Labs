#pragma once

#include <memory>
#include <string>

#include <fcntl.h>

#define syscall(...) _syscall(__VA_ARGS__)
// Includes missing from inline_executor.hpp
#include <boost/thread/thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>

#include <boost/thread/executors/inline_executor.hpp>
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

        virtual boost::future<int> ioctl(size_t request, memory::UserMemory argp);
};

class FileSystem {
    public:
        virtual ~FileSystem() = default;

        virtual boost::future<std::shared_ptr<File>> open(const std::string& pathname) = 0;
};

extern std::unique_ptr<FileSystem> rootFileSystem;
extern boost::inline_executor asyncExecutor;

}
