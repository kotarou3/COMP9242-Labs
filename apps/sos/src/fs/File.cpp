#include <stdexcept>
#include <system_error>

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

std::unique_ptr<FileSystem> rootFileSystem;
boost::inline_executor asyncExecutor;

}
