#include <limits>
#include <stdexcept>
#include <system_error>

#include <assert.h>
#include <dirent.h>

#include "internal/fs/File.h"

namespace fs {

boost::future<ssize_t> File::read(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
    throw std::invalid_argument("File not readable");
}

boost::future<ssize_t> File::write(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
    throw std::invalid_argument("File not writeable");
}

boost::future<int> File::ioctl(size_t /*request*/, memory::UserMemory /*argp*/) {
    throw std::invalid_argument("File not ioctl'able");
}

boost::future<ssize_t> Directory::read(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
    throw std::system_error(EISDIR, std::system_category(), "Directory not directly readable");
}

boost::future<ssize_t> Directory::write(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
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
boost::inline_executor asyncExecutor;

}
