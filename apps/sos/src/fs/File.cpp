#include <stdexcept>

#include "internal/fs/File.h"

namespace fs {

boost::future<ssize_t> File::read(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
    throw std::invalid_argument("File not readable");
}

boost::future<ssize_t> File::write(const std::vector<IoVector>& /*iov*/, off64_t /*offset*/) {
    throw std::invalid_argument("File not writeable");
}

boost::future<int> File::ioctl(size_t /*request*/, size_t /*arg*/) {
    throw std::invalid_argument("File not ioctl'able");
}

}
