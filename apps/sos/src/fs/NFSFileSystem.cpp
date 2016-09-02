#include "internal/fs/NFSFileSystem.h"
#include <memory>

#define NFS_CALLBACK(lambda, ...) make_unique<std::function<void(uintptr_t, ...)>>(lambda)}

namespace fs {

NFSFileSystem::NFSFileSystem() {
    if (nfs_mount(SOS_NFS_DIR, &mnt_point))
        throw std::runtime_error(std::string("Failed to mount nfs at") + SOS_NFS_DIR);
}

boost::future<std::shared_ptr<File>> NFSFileSystem::lookup(std::string path) {
    boost::promise<std::shared_ptr<File>> promise;
    if (lookupCache.find(path) != lookupCache.end())
        promise.set_value(lookupCache.find(path)->second);
    else
        auto lambda = [=](uintptr_t, enum nfs_stat status, fhandle_t *fh, fattr_t *fattr) {
            if (status == NFS_OK && fattr->type == NFNON) {
                // sos files can only be regular files
                lookupCache.emplace(path, std::make_shared<File>(fh, fattr));
                promise.set_value(lookupCache.find(path)->second);
            } else
                promise.set_value(nullptr);
            printf("Got status %d (NFS_OK = %d)\n", status, NFS_OK);
        };
        nfs_lookup(&mnt_point, path.c_str(),
                   NFS_CALLBACK(lambda, enum nfs_stat, fhandle_t*, fattr_t*), 0);
    return promise.get_future();
}

}