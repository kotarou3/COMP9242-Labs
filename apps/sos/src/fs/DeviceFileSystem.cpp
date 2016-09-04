#include <stdexcept>
#include <system_error>

#include "internal/fs/DeviceFileSystem.h"
#include "internal/syscall/helpers.h"

namespace fs {

boost::future<std::shared_ptr<File>> DeviceFileSystem::open(const std::string& pathname) {
    try {
        return _devices.at(pathname)();
    } catch (const std::out_of_range&) {
        std::throw_with_nested(std::system_error(ENOENT, std::system_category(), "No such device"));
    }
}

boost::future<std::unique_ptr<nfs::fattr_t>> DeviceFileSystem::stat(const std::string& pathname) {
    auto promise = std::make_shared<boost::promise<std::unique_ptr<nfs::fattr_t>>>();
    open(pathname).then(asyncExecutor, [promise] (auto result) {
        if (result.has_exception()) {
            promise->set_exception(syscall::getException(result));
        } else {
            promise->set_exception(syscall::err(ENOSYS));
        }
    });
    return promise->get_future();
}

void DeviceFileSystem::create(const std::string& name, const DeviceFileSystem::OpenCallback& openCallback) {
    auto result = _devices.insert(std::make_pair(name, openCallback));
    if (!result.second)
        throw std::invalid_argument("Device already exists");
}

}
