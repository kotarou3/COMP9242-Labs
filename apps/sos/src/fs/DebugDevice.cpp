#include <stdexcept>
#include <system_error>
#include <sys/ioctl.h>

#include "internal/fs/DebugDevice.h"

extern "C" {
    #include <sel4/sel4.h>
}

namespace fs {

boost::future<ssize_t> DebugDevice::write(const std::vector<IoVector>& iov, off64_t offset) {
    if (offset != CURRENT_OFFSET)
        throw std::system_error(ESPIPE, std::system_category(), "Cannot seek the debug device");

    ssize_t totalBytesWritten = 0;
    try {
        for (const auto& vector : iov) {
            auto map = memory::UserMemory(vector.buffer).mapIn<char>(vector.length, memory::Attributes{.read = true});
            for (size_t c = 0; c < vector.length; ++c)
                seL4_DebugPutChar(map.first[c]);
            totalBytesWritten += vector.length;
        }
    } catch (...) {
        if (totalBytesWritten == 0)
            throw;
    }

    return boost::make_ready_future(totalBytesWritten);
}

boost::future<int> DebugDevice::ioctl(size_t request, memory::UserMemory argp) {
    if (request != TIOCGWINSZ)
        throw std::invalid_argument("Unknown ioctl on debug device");

    // Can't query debug device for window size, so return a placeholder
    argp.set(winsize{
        .ws_row = 24,
        .ws_col = 80,
        .ws_xpixel = 640,
        .ws_ypixel = 480
    });

    return boost::make_ready_future(0);
}

}
