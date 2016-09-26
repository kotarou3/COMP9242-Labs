#include <stdexcept>
#include <system_error>
#include <sys/ioctl.h>

#include "internal/fs/DebugDevice.h"

extern "C" {
    #include <sel4/sel4.h>
}

namespace fs {

async::future<ssize_t> DebugDevice::_writeOne(const IoVector& iov, off64_t offset) {
    if (offset != CURRENT_OFFSET)
        throw std::system_error(ESPIPE, std::system_category(), "Cannot seek the debug device");

    return memory::UserMemory(iov.buffer).mapIn<char>(iov.length, memory::Attributes{.read = true})
        .then([iov](auto map) {
            auto _map = std::move(map.get());
            for (size_t c = 0; c < iov.length; ++c)
                seL4_DebugPutChar(_map.first[c]);
            return static_cast<ssize_t>(iov.length);
        });
}

async::future<int> DebugDevice::ioctl(size_t request, memory::UserMemory argp) {
    if (request != TIOCGWINSZ)
        throw std::invalid_argument("Unknown ioctl on debug device");

    // Can't query debug device for window size, so return a placeholder
    return argp.set(winsize{
        .ws_row = 24,
        .ws_col = 80,
        .ws_xpixel = 640,
        .ws_ypixel = 480
    }).then([](async::future<void> result) {
        result.get();
        return 0;
    });
}

}
