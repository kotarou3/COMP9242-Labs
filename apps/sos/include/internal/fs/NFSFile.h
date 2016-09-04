#pragma once

#include <nfs/nfs.h>

#include "internal/fs/File.h"

namespace fs {

class NFSFile : public File {
    public:
        virtual ~NFSFile() = default;

        virtual boost::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset) override;
        virtual boost::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset) override;

    private:
        NFSFile(const nfs::fhandle_t& handle);
        friend class NFSFileSystem;

        nfs::fhandle_t _handle;
        off64_t _currentOffset;
};

class NFSDirectory : public Directory {
    public:
        virtual ~NFSDirectory() = default;

        virtual boost::future<ssize_t> getdents(memory::UserMemory dirp, size_t length) override;

    private:
        NFSDirectory(const nfs::fhandle_t& handle);
        friend class NFSFileSystem;

        nfs::fhandle_t _handle;
        size_t _currentPosition;
};

}
