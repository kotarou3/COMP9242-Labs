#include <algorithm>
#include <limits>
#include <stdexcept>

#include "internal/fs/NFSFile.h"

namespace fs {

NFSFile::NFSFile(const nfs::fhandle_t& handle, const nfs::fattr_t& attrs):
    _handle(handle),
    _currentOffset(0),
    _attrs(attrs)
{}

boost::future<ssize_t> NFSFile::read(const std::vector<IoVector>& iov, off64_t offset) {
    // XXX: Only reads one IO vector
    IoVector currentIov = iov[0];

    off64_t actualOffset = offset;
    if (offset == fs::CURRENT_OFFSET)
        actualOffset = _currentOffset;
    if (actualOffset > std::numeric_limits<off_t>::max() - currentIov.length)
        throw std::invalid_argument("Final file offset overflows a off_t");

    return nfs::read(_handle, actualOffset, currentIov.length)
        .then(asyncExecutor, [this, currentIov](auto result) {
            auto _result = result.get();
            size_t read = std::min(std::get<1>(_result), currentIov.length);
            uint8_t* data = std::get<2>(_result);

            memory::UserMemory(currentIov.buffer).write(data, data + read);
            this->_currentOffset += read;

            return static_cast<ssize_t>(read);
        });
}

}
