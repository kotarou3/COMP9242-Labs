#pragma once

#include <memory>
#include <string>

#include <fcntl.h>

#include "internal/fs/File.h"
#include <nfs/nfs.h>

namespace fs {

class NFSFile : public File {
    public:
        NFSFile(fhandle_t* fh, fattr_t* fattr);
        virtual ~NFSFile() = default;

//        virtual boost::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset);
//        virtual boost::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset);
//
//        virtual boost::future<int> ioctl(size_t request, memory::UserMemory argp);
    private:
        fhandle_t handle;
};

}
