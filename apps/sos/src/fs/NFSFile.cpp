#include <algorithm>
#include <limits>
#include <stdexcept>

#include <dirent.h>

#include "internal/fs/NFSFile.h"

namespace fs {

/////////////
// NFSFile //
/////////////

NFSFile::NFSFile(const nfs::fhandle_t& handle):
    _handle(handle),
    _currentOffset(0)
{}

boost::future<ssize_t> NFSFile::read(const std::vector<IoVector>& iov, off64_t offset) {
    // XXX: Only reads one IO vector
    IoVector currentIov = iov[0];

    off64_t actualOffset = offset;
    if (offset == fs::CURRENT_OFFSET)
        actualOffset = _currentOffset;

    if (currentIov.length > std::numeric_limits<off_t>::max() ||
        actualOffset > std::numeric_limits<off_t>::max() - currentIov.length)
        throw std::invalid_argument("Final file offset overflows a off_t");

    auto map = std::make_shared<std::pair<uint8_t*, memory::ScopedMapping>>(
        memory::UserMemory(currentIov.buffer).mapIn<uint8_t>(
            currentIov.length,
            memory::Attributes{.read = false, .write = true}
        )
    );
    return nfs::read(_handle, actualOffset, currentIov.length, map->first)
        .then(asyncExecutor, [this, map](auto result) {
            size_t read = result.get();
            this->_currentOffset += read;

            return static_cast<ssize_t>(read);
        });
}

boost::future<ssize_t> NFSFile::write(const std::vector<IoVector>& iov, off64_t offset) {
    // XXX: Only writes one IO vector
    IoVector currentIov = iov[0];

    off64_t actualOffset = offset;
    if (offset == fs::CURRENT_OFFSET)
        actualOffset = _currentOffset;

    if (currentIov.length > std::numeric_limits<off_t>::max() ||
        actualOffset > std::numeric_limits<off_t>::max() - currentIov.length)
        throw std::invalid_argument("Final file offset overflows a off_t");

    auto map = std::make_shared<std::pair<uint8_t*, memory::ScopedMapping>>(
        memory::UserMemory(currentIov.buffer).mapIn<uint8_t>(
            currentIov.length,
            memory::Attributes{.read = true}
        )
    );
    return nfs::write(_handle, actualOffset, currentIov.length, map->first)
        .then(asyncExecutor, [this, map](auto result) {
            size_t written = result.get();
            this->_currentOffset += written;

            return static_cast<ssize_t>(written);
        });
}

//////////////////
// NFSDirectory //
//////////////////

NFSDirectory::NFSDirectory(const nfs::fhandle_t& handle):
    _handle(handle),
    _currentPosition(0)
{}

boost::future<ssize_t> NFSDirectory::getdents(memory::UserMemory dirp, size_t length) {
    length = std::min(length, static_cast<size_t>(std::numeric_limits<ssize_t>::max()));

    return nfs::readdir(_handle).then(asyncExecutor, [=](auto names) mutable {
        auto _names = names.get();
        auto map = dirp.mapIn<uint8_t>(length, memory::Attributes{.read = false, .write = true});

        dirent* curDirent = reinterpret_cast<dirent*>(map.first);
        size_t writtenBytes = 0;
        for (; this->_currentPosition < _names.size(); ++this->_currentPosition) {
            auto& name = _names[this->_currentPosition];

            dirent* nextDirent = _alignNextDirent(curDirent, name.size());
            size_t size = reinterpret_cast<size_t>(nextDirent) - reinterpret_cast<size_t>(curDirent);
            size_t nameLength = size - 1 - offsetof(dirent, d_name);

            if (size > length - writtenBytes)
                break;

            // XXX: Could fill out more by calling stat()
            curDirent->d_ino = 0;
            curDirent->d_off = 0;
            curDirent->d_reclen = size;
            curDirent->d_type = DT_UNKNOWN;
            curDirent->d_name[name.copy(curDirent->d_name, nameLength)] = 0;

            curDirent = nextDirent;
            writtenBytes += size;
        }

        if (writtenBytes == 0 && _currentPosition < _names.size())
            throw std::invalid_argument("Result buffer is too small");

        return static_cast<ssize_t>(writtenBytes);
    });
}

}
