#include <sys/uio.h>

#include "internal/fs/File.h"
#include "internal/memory/UserMemory.h"
#include "internal/syscall/fs.h"

#include "internal/fs/ConsoleDevice.h"
#include "internal/fs/DebugDevice.h"
#include "internal/fs/DeviceFileSystem.h"

namespace syscall {

namespace {
    boost::future<ssize_t> _preadwritev2(bool isWrite, int fd, const std::vector<fs::IoVector>& iov, off64_t offset) noexcept {
        (void)fd;

        if (iov.size() == 0)
            return _returnNow(0);

        // TODO: Do this properly
        /*static fs::DeviceFileSystem deviceFileSystem;
        static bool isInited = false;
        if (!isInited) {
            fs::ConsoleDevice::init(deviceFileSystem);
            isInited = true;
        }
        return deviceFileSystem.open("console").then([=](auto device) {
            if (isWrite)
                return device.get()->write(iov, offset);
            else
                return device.get()->read(iov, offset);
        });*/
        auto device = fs::DebugDevice();
        if (isWrite)
            return device.write(iov, offset);
        else
            return device.read(iov, offset);
    }

    std::vector<fs::IoVector> _getIovs(process::Process& process, memory::vaddr_t iov, size_t iovcnt) {
        if (iovcnt > IOV_MAX)
            throw std::invalid_argument("Too many IO vectors");

        std::vector<iovec> _iov(iovcnt);
        memory::UserMemory(process, iov).read(reinterpret_cast<uint8_t*>(_iov.data()), iovcnt * sizeof(iovec));

        std::vector<fs::IoVector> result;
        result.reserve(iovcnt);
        size_t totalLength = 0;
        for (const auto& vector : _iov) {
            size_t nextTotalLength = totalLength + vector.iov_len;
            if (nextTotalLength < totalLength || nextTotalLength > SSIZE_MAX)
                throw std::invalid_argument("Total amount of IO overflows a ssize_t");
            totalLength = nextTotalLength;

            if (vector.iov_len == 0)
                continue;

            result.push_back(fs::IoVector{
                .buffer = memory::UserMemory(process, reinterpret_cast<memory::vaddr_t>(vector.iov_base)),
                .length = vector.iov_len
            });
        }

        return result;
    }

    boost::future<ssize_t> _preadwritev(bool isWrite, process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) noexcept {
        try {
            return _preadwritev2(isWrite, fd, _getIovs(process, iov, iovcnt), offset);
        } catch (const std::invalid_argument&) {
            return _returnNow(-EINVAL);
        } catch (...) {
            return _returnNow(-EFAULT);
        }
    }

    boost::future<ssize_t> _preadwrite(bool isWrite, process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) noexcept {
        return _preadwritev2(
            isWrite, fd,
            std::vector<fs::IoVector>{fs::IoVector{
                .buffer = memory::UserMemory(process, buf),
                .length = count
            }},
            offset
        );
    }
}

boost::future<int> open(process::Process& process, const char* pathname, int flags, mode_t mode) noexcept {
    (void)process;
    (void)pathname;
    (void)flags;
    (void)mode;
    return _returnNow(-ENOSYS);
}

boost::future<int> close(process::Process& process, int fd) noexcept {
    (void)process;
    (void)fd;
    return _returnNow(-ENOSYS);
}

boost::future<int> read(process::Process& process, int fd, memory::vaddr_t buf, size_t count) noexcept {
    return _preadwrite(false, process, fd, buf, count, fs::CURRENT_OFFSET);
}
boost::future<int> readv(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt) noexcept {
    return _preadwritev(false, process, fd, iov, iovcnt, fs::CURRENT_OFFSET);
}
boost::future<int> pread(process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) noexcept {
    if (offset < 0)
        return _returnNow(-EINVAL);
    return _preadwrite(false, process, fd, buf, count, offset);
}
boost::future<int> preadv(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) noexcept {
    if (offset < 0)
        return _returnNow(-EINVAL);
    return _preadwritev(false, process, fd, iov, iovcnt, offset);
}

boost::future<int> write(process::Process& process, int fd, memory::vaddr_t buf, size_t count) noexcept {
    return _preadwrite(true, process, fd, buf, count, fs::CURRENT_OFFSET);
}
boost::future<int> writev(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt) noexcept {
    return _preadwritev(true, process, fd, iov, iovcnt, fs::CURRENT_OFFSET);
}
boost::future<int> pwrite(process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) noexcept {
    if (offset < 0)
        return _returnNow(-EINVAL);
    return _preadwrite(true, process, fd, buf, count, offset);
}
boost::future<int> pwritev(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) noexcept {
    if (offset < 0)
        return _returnNow(-EINVAL);
    return _preadwritev(true, process, fd, iov, iovcnt, offset);
}

boost::future<int> ioctl(process::Process& process, int fd, size_t request, size_t arg) noexcept {
    (void)process;
    (void)fd;
    (void)request;
    (void)arg;
    //return _returnNow(-ENOSYS);
    return _returnNow(0);
}

}

FORWARD_SYSCALL(open, 3);
FORWARD_SYSCALL(close, 1);

FORWARD_SYSCALL(read, 3);
FORWARD_SYSCALL(readv, 3);
FORWARD_SYSCALL(pread64, 6);
FORWARD_SYSCALL(preadv, 6);

FORWARD_SYSCALL(write, 3);
FORWARD_SYSCALL(writev, 3);
FORWARD_SYSCALL(pwrite64, 6);
FORWARD_SYSCALL(pwritev, 6);

FORWARD_SYSCALL(ioctl, 3);
