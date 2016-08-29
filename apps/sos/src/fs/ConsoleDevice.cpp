#include <stdexcept>

extern "C" {
    #include <serial/serial.h>
}

#include "internal/fs/ConsoleDevice.h"
#include "internal/fs/File.h"
#include "internal/fs/DeviceFileSystem.h"

namespace fs {

namespace {
    serial* _serial;
}

void ConsoleDevice::init(DeviceFileSystem& fs) {
    fs.create("console", [] {
        if (!_serial)
            _serial = serial_init();

        boost::promise<std::shared_ptr<File>> promise;
        promise.set_value(std::shared_ptr<File>(new ConsoleDevice));
        return promise.get_future();
    });
}

boost::future<ssize_t> ConsoleDevice::read(const std::vector<IoVector>& iov, off64_t offset) {
    if (offset != CURRENT_OFFSET)
        throw std::invalid_argument("Cannot seek the console device");

    // TODO
    (void)iov;
    (void)offset;

    boost::promise<ssize_t> promise;
    promise.set_value(0);
    return promise.get_future();
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

}
