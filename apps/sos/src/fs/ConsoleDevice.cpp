#include <deque>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

#include <sys/ioctl.h>

extern "C" {
    #include <serial/serial.h>
}

#include "internal/fs/ConsoleDevice.h"
#include "internal/fs/DeviceFileSystem.h"

namespace fs {

namespace {
    constexpr const size_t MAX_BUFFER_SIZE = 1048576;

    serial* _serial;

    std::deque<char> _readBuffer;
    std::queue<std::pair<std::vector<IoVector>, boost::promise<ssize_t>>> _readRequests;

    void _tryRead(std::deque<char>::const_iterator newlinePos) {
        static ssize_t _totalBytesRead = 0;
        static size_t _currentIovIndex = 0;

        while (_readRequests.size() > 0) {
            auto& request = _readRequests.front();
            try {
                for (; _currentIovIndex < request.first.size(); ++_currentIovIndex) {
                    auto& iov = request.first[_currentIovIndex];

                    size_t newlineIndex = -1U;
                    if (newlinePos != _readBuffer.cend())
                        newlineIndex = newlinePos - _readBuffer.cbegin();

                    size_t bytesToRead;
                    if (_readBuffer.size() >= iov.length)
                        bytesToRead = iov.length;
                    else if (newlineIndex != -1U)
                        bytesToRead = std::min(newlineIndex + 1, iov.length);
                    else
                        return;

                    iov.buffer.write(_readBuffer.cbegin(), _readBuffer.cbegin() + bytesToRead);
                    _readBuffer.erase(_readBuffer.cbegin(), _readBuffer.cbegin() + bytesToRead);
                    _totalBytesRead += bytesToRead;

                    if (newlineIndex != -1U && newlineIndex < bytesToRead)
                        newlinePos = std::find(_readBuffer.cbegin(), _readBuffer.cend(), '\n');
                }
            } catch (...) {
                if (_totalBytesRead == 0) {
                    request.second.set_value(-EFAULT);
                    _readRequests.pop();

                    _currentIovIndex = 0;
                    continue;
                }
            }

            request.second.set_value(_totalBytesRead);
            _readRequests.pop();

            _totalBytesRead = 0;
            _currentIovIndex = 0;
        }
    }

    void _serialHandler(serial* /*serial*/, char c) {
        _readBuffer.push_back(c);
        if (_readBuffer.size() > MAX_BUFFER_SIZE)
            _readBuffer.pop_front();

        if (!_readRequests.empty())
            _tryRead(_readBuffer.cend() - (c == '\n' ? 1 : 0));
    }
}

void ConsoleDevice::mountOn(DeviceFileSystem& fs, const std::string& name) {
    fs.create(name, [] {
        if (!_serial) {
            _serial = serial_init();
            serial_register_handler(_serial, _serialHandler);
        }

        boost::promise<std::shared_ptr<File>> promise;
        promise.set_value(std::shared_ptr<File>(new ConsoleDevice));
        return promise.get_future();
    });
}

boost::future<ssize_t> ConsoleDevice::read(const std::vector<IoVector>& iov, off64_t offset) {
    if (offset != CURRENT_OFFSET)
        throw std::invalid_argument("Cannot seek the console device");

    boost::promise<ssize_t> promise;
    auto future = promise.get_future();
    _readRequests.push(std::make_pair(iov, std::move(promise)));

    _tryRead(std::find(_readBuffer.cbegin(), _readBuffer.cend(), '\n'));

    return future;
}

boost::future<ssize_t> ConsoleDevice::write(const std::vector<IoVector>& iov, off64_t offset) {
    if (offset != CURRENT_OFFSET)
        throw std::invalid_argument("Cannot seek the console device");

    ssize_t totalBytesWritten = 0;
    try {
        for (const auto& vector : iov) {
            auto map = memory::UserMemory(vector.buffer).mapIn<char>(vector.length, memory::Attributes{.read = true});
            ssize_t bytesWritten = serial_send(_serial, map.first, vector.length);
            totalBytesWritten += bytesWritten;

            if (static_cast<size_t>(bytesWritten) != vector.length)
                break;
        }
    } catch (...) {
        if (totalBytesWritten == 0)
            throw;
    }

    boost::promise<ssize_t> promise;
    promise.set_value(totalBytesWritten);
    return promise.get_future();
}

boost::future<int> ConsoleDevice::ioctl(size_t request, memory::UserMemory argp) {
    if (request != TIOCGWINSZ)
        throw std::invalid_argument("Unknown ioctl on console device");

    // Can't query serial device for window size, so return a placeholder
    argp.set(winsize{
        .ws_row = 24,
        .ws_col = 80,
        .ws_xpixel = 640,
        .ws_ypixel = 480
    });

    boost::promise<ssize_t> promise;
    promise.set_value(0);
    return promise.get_future();
}

}
