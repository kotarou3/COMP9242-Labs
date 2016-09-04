#include "internal/fs/NFSFileSystem.h"
#include "internal/fs/NFSFile.h"
#include "internal/syscall/helpers.h"
#include <nfs/nfs.h>
#include <memory>

#include <stdexcept>


namespace fs {

NFSFileSystem::NFSFileSystem() {
    mnt_point = nfs::mount(SOS_NFS_DIR);
}

boost::future<std::unique_ptr<nfs::fattr_t>> NFSFileSystem::stat(const std::string& path) {
    boost::promise<std::unique_ptr<nfs::fattr_t>> promise;
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
    nfs::lookup(mnt_point, path.c_str()).then(asyncExecutor,
            [this, promise, path](auto results) {
                if (results.get().second->type == nfs::NFNON) {
                    // sos files can only be regular files
//                    this->lookupCache.emplace(path, std::make_shared<File>(fh, fattr));
//                    promise->set_value(this->lookupCache.find(path)->second);
                    try {
                        promise->set_value(std::make_shared<NFSFile>(results.get().first, results.get().second));
                    } catch (...) {
                        promise->set_exception(std::current_exception());
                    }
                } else
                    promise->set_exception(std::system_error(ENOENT, std::system_category(), "File was not a regular file"));
            }
    );
    return promise->get_future();
}

}