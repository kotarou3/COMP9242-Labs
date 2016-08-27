#include <sys/uio.h>

#include "internal/fs/File.h"
#include "internal/fs/FileDescriptor.h"
#include "internal/memory/UserMemory.h"
#include "internal/syscall/fs.h"

namespace syscall {

namespace {
    boost::future<ssize_t> _preadwritev2(bool isWrite, process::Process& process, int fd, const std::vector<fs::IoVector>& iov, off64_t offset) noexcept {
        if (iov.size() == 0)
            return _returnNow(0);

        fs::OpenFile::Flags flags = {
            .read = !isWrite,
            .write = isWrite
        };

        try {
            auto file = process.fdTable.get(fd, flags);
            if (isWrite)
                return file->write(iov, offset);
            else
                return file->read(iov, offset);
        } catch (...) {
            return _returnNow(-EBADF);
        }
    }

    std::vector<fs::IoVector> _getIovs(process::Process& process, memory::vaddr_t iov, size_t iovcnt) {
        if (iovcnt > IOV_MAX)
            throw std::invalid_argument("Too many IO vectors");

        auto _iov = memory::UserMemory(process, iov).get<iovec>(iovcnt);

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
            return _preadwritev2(isWrite, process, fd, _getIovs(process, iov, iovcnt), offset);
        } catch (const std::invalid_argument&) {
            return _returnNow(-EINVAL);
        } catch (...) {
            return _returnNow(-EFAULT);
        }
    }

    boost::future<ssize_t> _preadwrite(bool isWrite, process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) noexcept {
        return _preadwritev2(
            isWrite, process, fd,
            std::vector<fs::IoVector>{fs::IoVector{
                .buffer = memory::UserMemory(process, buf),
                .length = count
            }},
            offset
        );
    }
}

boost::future<int> open(process::Process& process, memory::vaddr_t pathname, int flags, mode_t mode) noexcept {
    fs::OpenFile::Flags openFileFlags = {0};
    int access = flags & O_ACCMODE;
    if (access == O_RDONLY) {
        openFileFlags.read = true;
    } else if (access == O_WRONLY) {
        openFileFlags.write = true;
    } else if (access == O_RDWR) {
        openFileFlags.read = true;
        openFileFlags.write = true;
    } else {
        return _returnNow(-EINVAL);
    }

    if (flags & ~O_ACCMODE)
        return _returnNow(-EINVAL);
    (void)mode;

    try {
        return fs::rootFileSystem->open(memory::UserMemory(process, pathname).readString())
            .then(fs::asyncExecutor, [openFileFlags, &process](auto file) {
                return process.fdTable.insert(std::make_shared<fs::OpenFile>(file.get(), openFileFlags));
            });
    } catch (...) {
        return _returnNow(-EINVAL);
    }
}

boost::future<int> close(process::Process& process, int fd) noexcept {
    if (process.fdTable.erase(fd))
        return _returnNow(0);
    else
        return _returnNow(-EBADF);
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

boost::future<int> ioctl(process::Process& process, int fd, size_t request, memory::vaddr_t argp) noexcept {
    try {
        return process.fdTable.get(fd, fs::OpenFile::Flags{})->ioctl(request, memory::UserMemory(process, argp));
    } catch (...) {
        return _returnNow(-EBADF);
    }
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
