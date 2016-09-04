#include <algorithm>
#include <limits>
#include <stdexcept>
#include <system_error>

#include <dirent.h>

#include "internal/fs/DeviceFileSystem.h"

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

        return boost::make_ready_future(result);
    } catch (const std::out_of_range&) {
        std::throw_with_nested(std::system_error(ENOENT, std::system_category(), "No such device"));
    }
}

boost::future<std::shared_ptr<File>> DeviceFileSystem::open(const std::string& pathname, OpenFlags flags) {
    if (pathname == ".")
        return boost::make_ready_future(std::shared_ptr<File>(new DeviceDirectory(*this)));

    try {
        return _devices.at(pathname).openCallback(flags);
    } catch (const std::out_of_range&) {
        std::throw_with_nested(std::system_error(ENOENT, std::system_category(), "No such device"));
    }
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

        dirent* nextDirent = _alignNextDirent(curDirent, name.size());
        size_t size = reinterpret_cast<size_t>(nextDirent) - reinterpret_cast<size_t>(curDirent);
        size_t nameLength = size - 1 - offsetof(dirent, d_name);

        if (size > length - writtenBytes)
            break;

        curDirent->d_ino = 0;
        curDirent->d_off = entryCount++;
        curDirent->d_reclen = size;
        curDirent->d_type = DT_CHR;
        curDirent->d_name[name.copy(curDirent->d_name, nameLength)] = 0;

        curDirent = nextDirent;
        writtenBytes += size;
    }

    if (writtenBytes == 0 && _currentPosition != _fs._devices.cend())
        throw std::invalid_argument("Result buffer is too small");

    return boost::make_ready_future(static_cast<ssize_t>(writtenBytes));
}

}
