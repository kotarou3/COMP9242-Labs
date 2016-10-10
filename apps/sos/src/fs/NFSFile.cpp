#include <algorithm>
#include <limits>
#include <stdexcept>

#include <dirent.h>

#include "internal/fs/NFSFile.h"

namespace {
    // No read pipelining because our ethernet driver sucks
    constexpr const size_t WRITE_MAX_PIPELINE_DEPTH = 16;
}

namespace fs {

/////////////
// NFSFile //
/////////////

NFSFile::NFSFile(const nfs::fhandle_t& handle):
    _handle(handle),
    _currentOffset(0)
{}

async::future<ssize_t> NFSFile::_readOne(const IoVector& iov, off64_t offset, bool bypassAttributes) {
    off64_t actualOffset = offset;
    if (offset == fs::CURRENT_OFFSET)
        actualOffset = _currentOffset;

    if (iov.length > std::numeric_limits<off_t>::max() ||
        actualOffset > std::numeric_limits<off_t>::max() - iov.length)
        throw std::invalid_argument("Final file offset overflows a off_t");

    return memory::UserMemory(iov.buffer).mapIn<uint8_t>(
        iov.length,
        memory::Attributes{.read = false, .write = true},
        bypassAttributes
    ).then([this, iov, offset, actualOffset](auto map) {
        auto _map = std::make_shared<std::pair<uint8_t*, memory::ScopedMapping>>(std::move(map.get()));

        return nfs::read(this->_handle, actualOffset, iov.length, _map->first)
            .then([this, offset, _map](auto result) {
                size_t read = result.get();
                if (offset == fs::CURRENT_OFFSET)
                    this->_currentOffset += read;

                return static_cast<ssize_t>(read);
            });
    });
}

async::future<ssize_t> NFSFile::_writeOne(const IoVector& iov, off64_t offset) {
    off64_t actualOffset = offset;
    if (offset == fs::CURRENT_OFFSET)
        actualOffset = _currentOffset;

    if (iov.length > std::numeric_limits<off_t>::max() ||
        actualOffset > std::numeric_limits<off_t>::max() - iov.length)
        throw std::invalid_argument("Final file offset overflows a off_t");

    return memory::UserMemory(iov.buffer).mapIn<uint8_t>(
        iov.length,
        memory::Attributes{.read = true}
    ).then([this, iov, offset, actualOffset](auto map) {
        auto _map = std::make_shared<std::pair<uint8_t*, memory::ScopedMapping>>(std::move(map.get()));

        size_t parallelWrites = std::max(std::min(iov.length / NFS_WRITE_BLOCK_SIZE, WRITE_MAX_PIPELINE_DEPTH), 1U);
        if (parallelWrites == 1) {
            return nfs::write(this->_handle, actualOffset, iov.length, _map->first)
                .then([this, offset, _map](auto result) {
                    size_t written = result.get();
                    if (offset == fs::CURRENT_OFFSET)
                        this->_currentOffset += written;

                    return static_cast<ssize_t>(written);
                });
        }

        std::vector<async::future<ssize_t>> writes;
        writes.reserve(parallelWrites);

        size_t writeOffset = 0;
        size_t writeLength = iov.length / parallelWrites;
        for (size_t n = 0; n < parallelWrites; ++n) {
            writes.push_back(
                nfs::write(this->_handle, actualOffset + writeOffset, writeLength, _map->first + writeOffset)
                    .then([this, offset, _map](auto result) {
                        size_t written = result.get();
                        if (offset == fs::CURRENT_OFFSET)
                            this->_currentOffset += written;

                        return static_cast<ssize_t>(written);
                    })
            );

            writeOffset += writeLength;
            if (n + 1 == parallelWrites)
                writeLength += iov.length - parallelWrites * writeLength;
        }

        return async::when_all(writes.begin(), writes.end()).then([](auto writes) {
            auto _writes = writes.get();
            ssize_t totalWritten = 0;
            for (auto& written : _writes)
                totalWritten += written.get();
            return totalWritten;
        });
    });
}

//////////////////
// NFSDirectory //
//////////////////

NFSDirectory::NFSDirectory(const nfs::fhandle_t& handle):
    _handle(handle),
    _currentPosition(0)
{}

async::future<ssize_t> NFSDirectory::getdents(memory::UserMemory dirp, size_t length) {
    length = std::min(length, static_cast<size_t>(std::numeric_limits<ssize_t>::max()));

    return async::when_all(
        nfs::readdir(_handle),
        dirp.mapIn<uint8_t>(length, memory::Attributes{.read = false, .write = true})
    ).then([=](auto results) mutable {
        auto _results = results.get();
        auto names = std::move(std::get<0>(_results).get());
        auto map = std::move(std::get<1>(_results).get());

        dirent* curDirent = reinterpret_cast<dirent*>(map.first);
        size_t writtenBytes = 0;
        for (; this->_currentPosition < names.size(); ++this->_currentPosition) {
            auto& name = names[this->_currentPosition];

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

        if (writtenBytes == 0 && _currentPosition < names.size())
            throw std::invalid_argument("Result buffer is too small");

        return static_cast<ssize_t>(writtenBytes);
    });
}

}
