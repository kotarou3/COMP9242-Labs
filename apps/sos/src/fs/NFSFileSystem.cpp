#include "internal/fs/NFSFileSystem.h"
#include <memory>


namespace fs {

NFSFileSystem::NFSFileSystem() {
    if (nfs_mount(SOS_NFS_DIR, &mnt_point))
        throw std::runtime_error(std::string("Failed to mount nfs at") + SOS_NFS_DIR);
}

boost::future<std::shared_ptr<File>> NFSFileSystem::lookup(std::string path) {
    auto promise = std::make_shared<boost::promise<std::shared_ptr<File>>>();
    if (lookupCache.find(path) != lookupCache.end())
        promise->set_value(lookupCache.find(path)->second);
    else
        nfs_lookup(&mnt_point, path.c_str(), std::make_unique<nfs_lookup_cb_t>(
                [this, promise, path](uintptr_t, enum nfs_stat status, fhandle_t *fh, fattr_t *fattr) {
                    if (status == NFS_OK && fattr->type == NFNON) {
                        // sos files can only be regular files
                        this->lookupCache.emplace(path, std::make_shared<File>(fh, fattr));
                        promise->set_value(this->lookupCache.find(path)->second);
                    } else
                        promise->set_value(nullptr);
                    printf("Got status %d (NFS_OK = %d)\n", status, NFS_OK);
                }
        ), 0);
    return promise->get_future();
}

}