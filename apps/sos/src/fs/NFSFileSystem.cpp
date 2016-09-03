#include "internal/fs/NFSFileSystem.h"
#include "internal/fs/NFSFile.h"
#include "internal/syscall/helpers.h"
#include <nfs/nfs.h>
#include <memory>
#include <stdexcept>


namespace fs {

NFSFileSystem::NFSFileSystem() {
    if (nfs_mount(SOS_NFS_DIR, &mnt_point))
        throw std::runtime_error(std::string("Failed to mount nfs at") + SOS_NFS_DIR);
}

boost::future<std::unique_ptr<fattr_t>> NFSFileSystem::stat(const std::string& path) {
    boost::promise<std::unique_ptr<fattr_t>> promise;
    auto f = lookup(path);
    promise.set_value(f.get()->getattrs());
    return promise.get_future();
}

boost::future<std::shared_ptr<File>> NFSFileSystem::open(const std::string& path) {
    return lookup(path);
}

boost::future<std::shared_ptr<File>> NFSFileSystem::lookup(const std::string& path) {
    std::cout << "Looking up " << path << std::endl;
    auto promise = std::make_shared<boost::promise<std::shared_ptr<File>>>();
    nfs_lookup(&mnt_point, path.c_str(), std::make_unique<nfs_lookup_cb_t>(
            [this, promise, path](enum nfs_stat status, fhandle_t *fh, fattr_t *fattr) {
                if (status == NFS_OK && fattr->type == NFNON) {
                    // sos files can only be regular files
//                    this->lookupCache.emplace(path, std::make_shared<File>(fh, fattr));
//                    promise->set_value(this->lookupCache.find(path)->second);
                    promise->set_value(std::make_shared<NFSFile>(fh, fattr));
                } else
                    promise->set_exception(syscall::err(ENOENT));
                printf("Got status %d (NFS_OK = %d)\n", status, NFS_OK);
            }
    ), 0);
    return promise->get_future();
}

}