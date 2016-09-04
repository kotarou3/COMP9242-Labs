#include <stdexcept>
#include <system_error>
#include <sys/uio.h>

#include "internal/fs/File.h"
#include "internal/fs/FileDescriptor.h"
#include "internal/memory/UserMemory.h"
#include "internal/syscall/fs.h"

#undef stat64

namespace syscall {

namespace {
    boost::future<ssize_t> _preadwritev2(bool isWrite, process::Process& process, int fd, const std::vector<fs::IoVector>& iov, off64_t offset) {
        if (iov.size() == 0)
            return _returnNow(0);

        fs::OpenFile::Flags flags = {
            .read = !isWrite,
            .write = isWrite
        };

        auto file = process.fdTable.get(fd, flags);
        if (isWrite)
            return file->write(iov, offset);
        else
            return file->read(iov, offset);
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

    boost::future<ssize_t> _preadwritev(bool isWrite, process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) {
        return _preadwritev2(isWrite, process, fd, _getIovs(process, iov, iovcnt), offset);
    }

    boost::future<ssize_t> _preadwrite(bool isWrite, process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) {
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

boost::future<int> stat64(process::Process& process, memory::vaddr_t pathname, memory::vaddr_t buf) {
    return fs::rootFileSystem->stat(memory::UserMemory(process, pathname).readString())
        .then(fs::asyncExecutor, [&process, buf](auto stat) mutable {
            memory::UserMemory(process, buf).set(stat.get());
            return 0;
        });
}

boost::future<int> open(process::Process& process, memory::vaddr_t pathname, int flags, mode_t mode) {
    fs::FileSystem::OpenFlags openFlags = {0};
    int access = flags & O_ACCMODE;
    if (access == O_RDONLY) {
        openFlags.read = true;
    } else if (access == O_WRONLY) {
        openFlags.write = true;
    } else if (access == O_RDWR) {
        openFlags.read = true;
        openFlags.write = true;
    } else {
        throw std::invalid_argument("File must be opened with some permissions");
    }

    // XXX: Ignore these flags for now
    flags &= ~(O_TRUNC | O_LARGEFILE | O_CLOEXEC);

    openFlags.createOnMissing = flags & O_CREAT;
    openFlags.mode = mode;

    if (flags & ~(O_ACCMODE | O_CREAT | O_DIRECTORY))
        throw std::invalid_argument("Invalid flags");
    if (mode & ~07777)
        throw std::invalid_argument("Invalid mode");

    return fs::rootFileSystem->open(memory::UserMemory(process, pathname).readString(), openFlags)
        .then(fs::asyncExecutor, [flags, openFlags, &process](auto file) {
            auto _file = file.get();

            if ((flags & O_DIRECTORY) && !dynamic_cast<fs::Directory*>(_file.get()))
                throw std::system_error(ENOTDIR, std::system_category(), "O_DIRECTORY set but file isn't a directory");

            return process.fdTable.insert(std::make_shared<fs::OpenFile>(
                _file,
                fs::OpenFile::Flags{.read = openFlags.read, .write = openFlags.write}
            ));
        });
}

boost::future<int> close(process::Process& process, int fd) {
    if (process.fdTable.erase(fd))
        return _returnNow(0);

    throw std::system_error(EBADF, std::system_category());
}

boost::future<int> read(process::Process& process, int fd, memory::vaddr_t buf, size_t count) {
    return _preadwrite(false, process, fd, buf, count, fs::CURRENT_OFFSET);
}
boost::future<int> readv(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt) {
    return _preadwritev(false, process, fd, iov, iovcnt, fs::CURRENT_OFFSET);
}
boost::future<int> pread(process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) {
    if (offset < 0)
        throw std::invalid_argument("Invalid offset");
    return _preadwrite(false, process, fd, buf, count, offset);
}
boost::future<int> preadv(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) {
    if (offset < 0)
        throw std::invalid_argument("Invalid offset");
    return _preadwritev(false, process, fd, iov, iovcnt, offset);
}

boost::future<int> write(process::Process& process, int fd, memory::vaddr_t buf, size_t count) {
    return _preadwrite(true, process, fd, buf, count, fs::CURRENT_OFFSET);
}
boost::future<int> writev(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt) {
    return _preadwritev(true, process, fd, iov, iovcnt, fs::CURRENT_OFFSET);
}
boost::future<int> pwrite(process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) {
    if (offset < 0)
        throw std::invalid_argument("Invalid offset");
    return _preadwrite(true, process, fd, buf, count, offset);
}
boost::future<int> pwritev(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) {
    if (offset < 0)
        throw std::invalid_argument("Invalid offset");
    return _preadwritev(true, process, fd, iov, iovcnt, offset);
}

boost::future<int> getdents64(process::Process& process, int fd, memory::vaddr_t dirp, size_t count) {
    auto file = process.fdTable.get(fd, fs::OpenFile::Flags{});
    auto directory = dynamic_cast<fs::Directory*>(file.get());
    if (!directory)
        throw std::system_error(ENOTDIR, std::system_category());

    return directory->getdents(memory::UserMemory(process, dirp), count);
}

boost::future<int> fcntl64(process::Process& process, int fd, int cmd, int arg) {
    process.fdTable.get(fd, fs::OpenFile::Flags{});

    if (cmd == F_SETFD && arg == FD_CLOEXEC)
        return _returnNow(0); // XXX: Ignore for now

    throw std::system_error(ENOSYS, std::system_category());
}

boost::future<int> ioctl(process::Process& process, int fd, size_t request, memory::vaddr_t argp) {
    return process.fdTable.get(fd, fs::OpenFile::Flags{})->ioctl(request, memory::UserMemory(process, argp));
}

}

FORWARD_SYSCALL(stat64, 2);
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

FORWARD_SYSCALL(getdents64, 3);

FORWARD_SYSCALL(fcntl64, 3);
FORWARD_SYSCALL(ioctl, 3);
