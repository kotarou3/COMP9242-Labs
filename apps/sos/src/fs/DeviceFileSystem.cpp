#include <stdexcept>

#include "internal/fs/DeviceFileSystem.h"

namespace fs {

boost::future<std::shared_ptr<File>> DeviceFileSystem::open(const std::string& pathname) {
    return _devices.at(pathname)();
}

void DeviceFileSystem::create(const std::string& name, const DeviceFileSystem::OpenCallback& openCallback) {
    auto result = _devices.insert(std::make_pair(name, openCallback));
    if (!result.second)
        throw std::invalid_argument("Device already exists");
}

}
