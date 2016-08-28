#include <stdexcept>

#include "internal/fs/DebugDevice.h"

extern "C" {
    #include <sel4/sel4.h>
}

namespace fs {

boost::future<ssize_t> DebugDevice::write(const std::vector<IoVector>& iov, off64_t offset) {
    if (offset != CURRENT_OFFSET)
        throw std::invalid_argument("Cannot seek the debug device");

    ssize_t totalBytesWritten = 0;
    for (const auto& vector : iov) {
        std::vector<uint8_t> buffer = memory::UserMemory(vector.buffer).read(vector.length);
        for (char c : buffer)
            seL4_DebugPutChar(c);
        totalBytesWritten += vector.length;
    }

    boost::promise<ssize_t> promise;
    promise.set_value(totalBytesWritten);
    return promise.get_future();
}

}
