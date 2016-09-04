#pragma once

#include <memory>
#include <string>

#include <fcntl.h>

#include "internal/fs/File.h"
#include <nfs/nfs.h>

namespace fs {

class NFSFile : public File {
    public:
        NFSFile(const nfs::fhandle_t* fh, const nfs::fattr_t* attributes);
        virtual ~NFSFile() = default;

        virtual std::unique_ptr<nfs::fattr_t> getattrs() override {return std::make_unique<nfs::fattr_t>(attrs);}

//        virtual boost::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset);
//        virtual boost::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset);
//
//        virtual boost::future<int> ioctl(size_t request, memory::UserMemory argp);
    private:
        nfs::fhandle_t handle;

        // temporary until we have an actual cache
        nfs::fattr_t attrs;
};

}
