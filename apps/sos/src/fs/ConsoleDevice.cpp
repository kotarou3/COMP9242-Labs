#include <deque>
#include <queue>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include <boost/circular_buffer.hpp>

#include <sys/ioctl.h>

extern "C" {
    #include <serial/serial.h>
}

#include "internal/fs/ConsoleDevice.h"
#include "internal/fs/DeviceFileSystem.h"

namespace fs {

namespace {
    constexpr const size_t MIN_BUFFER_SIZE = 8192;
    constexpr const size_t MAX_BUFFER_SIZE = 1048576;

    serial* _serial;

    boost::circular_buffer_space_optimized<char> _readBuffer(
        boost::circular_buffer_space_optimized<char>::capacity_type(MAX_BUFFER_SIZE, MIN_BUFFER_SIZE)
    );
    std::queue<std::pair<IoVector, async::promise<ssize_t>>> _readRequests;

    void _tryRead(decltype(_readBuffer)::iterator newlinePos) noexcept {
        // TODO: Better mutual exclusion
        static bool _isExecuting;
        if (_readRequests.empty() || _isExecuting)
            return;
        _isExecuting = true;

        auto& request = _readRequests.front();
        IoVector& iov = request.first;

        size_t newlineIndex = -1U;
        if (newlinePos != _readBuffer.end())
            newlineIndex = newlinePos - _readBuffer.begin();

        size_t bytesToRead;
        if (_readBuffer.size() >= iov.length) {
            bytesToRead = iov.length;
        } else if (newlineIndex != -1U) {
            bytesToRead = std::min(newlineIndex + 1, iov.length);
        } else {
            _isExecuting = false;
            return;
        }

        try {
            iov.buffer.write(_readBuffer.begin(), _readBuffer.begin() + bytesToRead)
                .then([&request, newlineIndex, newlinePos, bytesToRead](async::future<void> result) {
                    _isExecuting = false;

                    try {
                        result.get();

                        _readBuffer.erase(_readBuffer.begin(), _readBuffer.begin() + bytesToRead);

                        request.second.set_value(bytesToRead);
                        _readRequests.pop();

                        if (newlineIndex != -1U && newlineIndex < bytesToRead)
                            _tryRead(std::find(_readBuffer.begin(), _readBuffer.end(), '\n'));
                        else
                            _tryRead(newlinePos);
                    } catch (...) {
                        request.second.set_exception(std::current_exception());
                        _readRequests.pop();

                        _tryRead(newlinePos);
                    }
                });
        } catch (...) {
            _isExecuting = false;

            request.second.set_exception(std::current_exception());
            _readRequests.pop();

            _tryRead(newlinePos);
        }
    }

    void _serialHandler(serial* /*serial*/, char c) noexcept {
        try {
            _readBuffer.push_back(c);
            if (!_readRequests.empty())
                _tryRead(_readBuffer.end() - (c == '\n' ? 1 : 0));
        } catch (const std::bad_alloc&) {
            _readBuffer.pop_front();
            _serialHandler(nullptr, c);
        }
    }
}

void ConsoleDevice::mountOn(DeviceFileSystem& fs, const std::string& name) {
    fs.create(name, [](auto /*flags*/) {
        if (!_serial) {
            _serial = serial_init();
            serial_register_handler(_serial, _serialHandler);
        }

        return async::make_ready_future(std::shared_ptr<File>(new ConsoleDevice));
    });
}

async::future<ssize_t> ConsoleDevice::_readOne(const IoVector& iov, off64_t offset) {
    if (offset != CURRENT_OFFSET)
        throw std::system_error(ESPIPE, std::system_category(), "Cannot seek the console device");

    async::promise<ssize_t> promise;
    auto future = promise.get_future();
    _readRequests.push(std::make_pair(iov, std::move(promise)));

    _tryRead(std::find(_readBuffer.begin(), _readBuffer.end(), '\n'));

    return future;
}

async::future<ssize_t> ConsoleDevice::_writeOne(const IoVector& iov, off64_t offset) {
    if (offset != CURRENT_OFFSET)
        throw std::system_error(ESPIPE, std::system_category(), "Cannot seek the console device");

    return memory::UserMemory(iov.buffer).mapIn<char>(iov.length, memory::Attributes{.read = true})
        .then([iov](auto map) {
            auto _map = std::move(map.get());
            return serial_send(_serial, _map.first, iov.length);
        });
}

async::future<int> ConsoleDevice::ioctl(size_t request, memory::UserMemory argp) {
    if (request != TIOCGWINSZ)
        throw std::invalid_argument("Unknown ioctl on console device");

    // Can't query serial device for window size, so return a placeholder
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
