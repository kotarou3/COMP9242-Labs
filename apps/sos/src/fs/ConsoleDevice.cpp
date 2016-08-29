#include <deque>
#include <utility>
#include <queue>
#include <vector>
#include <stdexcept>
#include <sys/ioctl.h>

extern "C" {
    #include <serial/serial.h>
}

#include "internal/fs/ConsoleDevice.h"
#include "internal/fs/File.h"
#include "internal/fs/DeviceFileSystem.h"

namespace fs {

namespace {
    serial* _serial;
    auto buffer = std::deque<char>{};
    auto requests = std::queue<std::pair<std::vector<IoVector>, boost::promise<ssize_t>>>{};
    constexpr const unsigned int MAX_BUFFER_SIZE = 1<<18; // 1MB

    inline void tryRead(bool forceFlush=false) {
        static int total = 0;
        static size_t index = 0U;
        auto& iov = requests.front().first[index];
        if (forceFlush || buffer.size() >= iov.length - 1) {
            int size = std::min(buffer.size(), iov.length);
            iov.buffer.write(buffer.cbegin(), buffer.cbegin() + size, false);
            total += size;
            if (++index == requests.front().first.size()) {
                requests.front().second.set_value(total);
                total = 0;
                index = 0U;
                requests.pop();
            }
        }
    }

    void serialHandler(serial*, char c) {
        buffer.push_back(c);
        if (buffer.size() > MAX_BUFFER_SIZE)
            buffer.pop_front(); // just start dropping the stuff at the front
        if (c == '\n' && requests.empty())
            buffer.clear();
        else if (!requests.empty()) {
            tryRead(c == '\n');
        }
    }
}

void ConsoleDevice::mountOn(DeviceFileSystem& fs, const std::string& name) {
    fs.create(name, [] {
        if (!_serial) {
            _serial = serial_init();
            serial_register_handler(_serial, serialHandler);
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
    boost::future<ssize_t> future = promise.get_future();
    requests.push(std::make_pair(iov, std::move(promise)));
    tryRead();
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
