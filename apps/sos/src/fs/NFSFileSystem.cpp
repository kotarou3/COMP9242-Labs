#include <stdexcept>

extern "C" {
    #include <autoconf.h>
}

#include "internal/fs/NFSFile.h"
#include "internal/fs/NFSFileSystem.h"

namespace fs {

NFSFileSystem::NFSFileSystem(const std::string& serverIp, const std::string& nfsDir) {
    ip_addr ip;
    if (!ipaddr_aton(CONFIG_SOS_GATEWAY, &ip))
        throw std::invalid_argument(serverIp + " is not a valid IPv4 address");
    printf("NFS initialising\n");
    nfs::init(ip);

    printf("NFS initialised\n");
    _handle = nfs::mount(nfsDir);
    printf("NFS mounted\n");
    _timeoutTimer = timer::setTimer(std::chrono::milliseconds(100), nfs::timeout, true);
}


boost::future<std::unique_ptr<nfs::fattr_t>> NFSFileSystem::stat(const std::string& path) {
    boost::promise<std::unique_ptr<nfs::fattr_t>> promise;
    auto f = open(path);
    promise.set_value(f.get()->getattrs());
    return promise.get_future();
}

NFSFileSystem::~NFSFileSystem() {
    timer::clearTimer(_timeoutTimer);
}

boost::future<std::shared_ptr<File>> NFSFileSystem::open(const std::string& pathname) {
    return nfs::lookup(_handle, pathname)
        .then(asyncExecutor, [](auto result) {
            return std::shared_ptr<File>(new NFSFile(*result.get().first, *result.get().second));
        });
}

}
