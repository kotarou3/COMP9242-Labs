#pragma once

#include "internal/fs/File.h"
#include <nfs/nfs.h>

#ifndef SOS_NFS_DIR
#  ifdef CONFIG_SOS_NFS_DIR
#    define SOS_NFS_DIR CONFIG_SOS_NFS_DIR
#  else
#    define SOS_NFS_DIR ""
#  endif
#endif


namespace fs {

class NFSFileSystem : public FileSystem {
public:
    NFSFileSystem();

    virtual ~NFSFileSystem() override = default;

    // can't copy or move. This way it can be passed as a reference to the lambda functions
    NFSFileSystem(const NFSFileSystem&) = delete;
    NFSFileSystem(NFSFileSystem&&) = delete;
    NFSFileSystem& operator=(const NFSFileSystem&) = delete;
    NFSFileSystem& operator=(NFSFileSystem&&) = delete;

    virtual boost::future <std::shared_ptr<File>> open(const std::string &pathname) override;
    boost::future <std::shared_ptr<File>> lookup(const std::string &pathname);
    virtual boost::future <std::unique_ptr<nfs::fattr_t>> stat(const std::string &pathname) override;

private:
    nfs::fhandle_t mnt_point = {{0}};
};

}