#include <stdexcept>

#include "internal/fs/DeviceFileSystem.h"
#include "internal/syscall/helpers.h"

namespace fs {

boost::future<std::shared_ptr<File>> DeviceFileSystem::open(const std::string& pathname) {
    return _devices.at(pathname)();
}

boost::future<std::unique_ptr<fattr_t>> DeviceFileSystem::stat(const std::string& pathname) {
    (void)pathname;
    throw syscall::err(ENOSYS);
}

void DeviceFileSystem::create(const std::string& name, const DeviceFileSystem::OpenCallback& openCallback) {
    auto result = _devices.insert(std::make_pair(name, openCallback));
    if (!result.second)
        throw std::invalid_argument("Device already exists");
}

}
