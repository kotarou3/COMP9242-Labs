#pragma once

#include <memory>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>

#define syscall(...) _syscall(__VA_ARGS__)
// Includes missing from inline_executor.hpp
#include <boost/thread/thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>

#include <boost/thread/executors/inline_executor.hpp>
#include <boost/thread/future.hpp>
#undef syscall

#include "internal/memory/UserMemory.h"

struct dirent;

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

class Directory : public File {
    public:
        virtual ~Directory() = default;

        virtual boost::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset) override;
        virtual boost::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset) override;

        virtual boost::future<ssize_t> getdents(memory::UserMemory dirp, size_t length) = 0;

    protected:
        static dirent* _alignNextDirent(dirent* curDirent, size_t nameLength);
};

class FileSystem {
    public:
        struct OpenFlags {
            bool read:1, write:1;
            bool createOnMissing:1, truncate:1;
            mode_t mode;
        };

        virtual ~FileSystem() = default;

        virtual boost::future<struct stat> stat(const std::string& pathname) = 0;
        virtual boost::future<std::shared_ptr<File>> open(const std::string& pathname, OpenFlags flags) = 0;
};

extern std::unique_ptr<FileSystem> rootFileSystem;
extern boost::inline_executor asyncExecutor;

}
