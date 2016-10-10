#include <limits>
#include <stdexcept>
#include <system_error>

#include <assert.h>
#include <dirent.h>

#include "internal/fs/File.h"

namespace fs {

async::future<ssize_t> File::read(const std::vector<IoVector>& iov, off64_t offset, bool bypassAttributes) {
    if (iov.size() == 1) {
        // Fast common case of single iov
        return this->_readOne(iov[0], offset, bypassAttributes);
    }

    auto future = async::make_ready_future<ssize_t>(0);

    ssize_t expectedBytesRead = 0;
    for (const auto& vector : iov) {
        future = future.then([=](auto read) {
            ssize_t _read = read.get();
            if (_read != expectedBytesRead)
                return async::make_ready_future(_read);

            return this->_readOne(vector, offset == CURRENT_OFFSET ? CURRENT_OFFSET : offset + _read, bypassAttributes);
        }).unwrap().then([expectedBytesRead](auto read) {
            try {
                return expectedBytesRead + read.get();
            } catch (...) {
                if (expectedBytesRead != 0)
                    throw;
                return expectedBytesRead;
            }
        });

        expectedBytesRead += vector.length;
    }

    return future;
}

async::future<ssize_t> File::write(const std::vector<IoVector>& iov, off64_t offset) {
    if (iov.size() == 1) {
        // Fast common case of single iov
        return this->_writeOne(iov[0], offset);
    }

    auto future = async::make_ready_future<ssize_t>(0);

    ssize_t expectedBytesWritten = 0;
    for (const auto& vector : iov) {
        future = future.then([this, vector, offset, expectedBytesWritten](auto written) {
            ssize_t _written = written.get();
            if (_written != expectedBytesWritten)
                return async::make_ready_future(_written);

            return this->_writeOne(vector, offset == CURRENT_OFFSET ? CURRENT_OFFSET : offset + _written);
        }).unwrap().then([expectedBytesWritten](auto written) {
            try {
                return expectedBytesWritten + written.get();
            } catch (...) {
                if (expectedBytesWritten != 0)
                    throw;
                return expectedBytesWritten;
            }
        });

        expectedBytesWritten += vector.length;
    }

    return future;
}

async::future<int> File::ioctl(size_t /*request*/, memory::UserMemory /*argp*/) {
    throw std::invalid_argument("File not ioctl'able");
}

async::future<ssize_t> File::_readOne(const IoVector& /*iov*/, off64_t /*offset*/, bool /*bypassAttributes*/) {
    throw std::invalid_argument("File not readable");
}

async::future<ssize_t> File::_writeOne(const IoVector& /*iov*/, off64_t /*offset*/) {
    throw std::invalid_argument("File not writeable");
}

async::future<ssize_t> Directory::read(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/, bool /*bypassAttributes*/) {
    throw std::system_error(EISDIR, std::system_category(), "Directory not directly readable");
}

async::future<ssize_t> Directory::write(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
    throw std::system_error(EISDIR, std::system_category(), "Directory not directly writable");
}

dirent* Directory::_alignNextDirent(dirent* curDirent, size_t nameLength) {
    size_t size = std::min(
        offsetof(dirent, d_name) + nameLength + 1,
        static_cast<size_t>(std::numeric_limits<decltype(curDirent->d_reclen)>::max()) & -alignof(dirent)
    );

    dirent* nextDirent = reinterpret_cast<dirent*>(reinterpret_cast<size_t>(curDirent) + size);
    size_t remainingSize = 2 * alignof(dirent); // Dummy value
    assert(std::align(alignof(dirent), 1, reinterpret_cast<void*&>(nextDirent), remainingSize));

    assert(reinterpret_cast<size_t>(nextDirent) - reinterpret_cast<size_t>(curDirent) < static_cast<size_t>(std::numeric_limits<decltype(curDirent->d_reclen)>::max()));

    return nextDirent;
}

std::unique_ptr<FileSystem> rootFileSystem;

}
