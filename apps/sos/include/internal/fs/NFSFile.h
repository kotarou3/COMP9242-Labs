#pragma once

#include <nfs/nfs.h>

#include "internal/fs/File.h"
#include <memory>
#include <string>
#include <fcntl.h>

namespace fs {

class NFSFile : public File {
    public:
        virtual ~NFSFile() = default;

        virtual boost::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset) override;

        virtual std::unique_ptr<nfs::fattr_t> getattrs() override {return std::make_unique<nfs::fattr_t>(_attrs);}
    private:
        NFSFile(const nfs::fhandle_t& handle, const nfs::fattr_t* attrs);
        friend class NFSFileSystem;

        nfs::fhandle_t _handle;
        off64_t _currentOffset;

        nfs_fattrs_t _attrs
};

}
