#include <algorithm>
#include <limits>
#include <stdexcept>
#include <system_error>

#include <dirent.h>

#include "internal/fs/DeviceFileSystem.h"
#include "internal/syscall/helpers.h"

namespace fs {

//////////////////////
// DeviceFileSystem //
//////////////////////

boost::future<struct stat> DeviceFileSystem::stat(const std::string& pathname) {
    try {
        auto device = _devices.at(pathname);

        struct stat result = {0};
        result.st_mode = 0666 | S_IFCHR;
        result.st_nlink = 1;
        result.st_blksize = PAGE_SIZE;
        result.st_atim = device.accessTime;
        result.st_mtim = device.modifyTime;
        result.st_ctim = device.changeTime;

        boost::promise<struct stat> promise;
        promise.set_value(result);
        return promise.get_future();
    } catch (const std::out_of_range&) {
        std::throw_with_nested(std::system_error(ENOENT, std::system_category(), "No such device"));
    }
}

boost::future<std::shared_ptr<File>> DeviceFileSystem::open(const std::string& pathname, OpenFlags flags) {
    if (pathname == ".") {
        boost::promise<std::shared_ptr<File>> promise;
        promise.set_value(std::make_shared<DeviceDirectory>(*this));
        return promise.get_future();
    }

    try {
        return _devices.at(pathname).openCallback(flags);
    } catch (const std::out_of_range&) {
        std::throw_with_nested(std::system_error(ENOENT, std::system_category(), "No such device"));
    }
}

boost::future<std::unique_ptr<nfs::fattr_t>> DeviceFileSystem::stat(const std::string& pathname) {
    auto promise = boost::promise<std::unique_ptr<nfs::fattr_t>>();
    open(pathname).then(asyncExecutor, [] (auto result) {
        // TODO: actually check read / write attributes of the device
        (void)result;
        return std::unique_ptr<nfs::fattr_t>(new nfs::fattr_t{});
    });
    return promise.get_future();
}

void DeviceFileSystem::create(const std::string& name, const DeviceFileSystem::OpenCallback& openCallback) {
    timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0)
        throw std::system_error(errno, std::system_category(), "Failed to get current time");

    auto result = _devices.insert(std::make_pair(name, Device{
        .openCallback = openCallback,
        .accessTime = now,
        .modifyTime = now,
        .changeTime = now
    }));
    if (!result.second)
        throw std::invalid_argument("Device already exists");
}

/////////////////////
// DeviceDirectory //
/////////////////////

DeviceDirectory::DeviceDirectory(const DeviceFileSystem& fs):
    _fs(fs),
    _currentPosition(fs._devices.cbegin())
{}

boost::future<ssize_t> DeviceDirectory::getdents(memory::UserMemory dirp, size_t length) {
    length = std::min(length, static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
    auto map = dirp.mapIn<uint8_t>(length, memory::Attributes{.read = false, .write = true});

    dirent* curDirent = reinterpret_cast<dirent*>(map.first);
    size_t writtenBytes = 0;
    size_t entryCount = 0;
    for (; _currentPosition != _fs._devices.cend(); ++_currentPosition) {
        auto& name = _currentPosition->first;

        size_t size = std::min(
            offsetof(dirent, d_name) + name.size() + 1,
            static_cast<size_t>(std::numeric_limits<decltype(curDirent->d_reclen)>::max())
        );
        if (writtenBytes + size > length)
            break;

        *curDirent = {0};
        curDirent->d_off = entryCount++;
        curDirent->d_reclen = size;
        curDirent->d_type = DT_CHR;

        size_t nameLength = size - 1 - offsetof(dirent, d_name);
        name.copy(curDirent->d_name, nameLength);
        curDirent->d_name[nameLength] = 0;

        curDirent = reinterpret_cast<dirent*>(reinterpret_cast<uint8_t*>(curDirent) + size);
        writtenBytes += size;
    }

    if (writtenBytes == 0 && _currentPosition != _fs._devices.cend())
        throw std::invalid_argument("Result buffer is too small");

    boost::promise<ssize_t> promise;
    promise.set_value(writtenBytes);
    return promise.get_future();
}

}
