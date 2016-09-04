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
    nfs::init(ip);

    _handle = nfs::mount(nfsDir);
    _timeoutTimer = timer::setTimer(std::chrono::milliseconds(100), nfs::timeout, true);
}

NFSFileSystem::~NFSFileSystem() {
    timer::clearTimer(_timeoutTimer);
}

boost::future<std::shared_ptr<File>> NFSFileSystem::open(const std::string& pathname) {
    return nfs::lookup(_handle, pathname)
        .then(asyncExecutor, [](auto result) {
            return std::shared_ptr<File>(new NFSFile(*result.get().first));
        });
}

}
