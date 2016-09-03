#include <stdexcept>

#include "internal/fs/File.h"
#include "internal/syscall/helpers.h"


namespace fs {

boost::future<ssize_t> File::read(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
    throw std::invalid_argument("File not readable");
}

boost::future<ssize_t> File::write(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
    throw std::invalid_argument("File not writeable");
}

std::unique_ptr<fattr_t> File::getattrs() {throw syscall::err(ENOSYS);}

boost::future<int> File::ioctl(size_t /*request*/, memory::UserMemory /*argp*/) {
    throw std::invalid_argument("File not ioctl'able");
}

std::unique_ptr<FileSystem> rootFileSystem;
boost::inline_executor asyncExecutor;

}
