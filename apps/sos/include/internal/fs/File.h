#pragma once

#include <memory>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>

#include "internal/async.h"
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

        virtual async::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset, bool bypassAttributes = false);
        virtual async::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset);

        virtual async::future<int> ioctl(size_t request, memory::UserMemory argp);

    protected:
        virtual async::future<ssize_t> _readOne(const IoVector& iov, off64_t offset, bool bypassAttributes);
        virtual async::future<ssize_t> _writeOne(const IoVector& iov, off64_t offset);
};

class Directory : public File {
    public:
        virtual ~Directory() = default;

        virtual async::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset, bool bypassAttributes) override;
        virtual async::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset) override;

        virtual async::future<ssize_t> getdents(memory::UserMemory dirp, size_t length) = 0;

    protected:
        static dirent* _alignNextDirent(dirent* curDirent, size_t nameLength);
};

class FileSystem {
    public:
        struct OpenFlags {
            bool read:1, write:1;

            bool direct:1;
            bool createOnMissing:1;
            mode_t mode;
        };

        virtual ~FileSystem() = default;

        virtual async::future<struct stat> stat(const std::string& pathname) = 0;
        virtual async::future<std::shared_ptr<File>> open(const std::string& pathname, OpenFlags flags) = 0;
};

extern std::unique_ptr<FileSystem> rootFileSystem;

}
