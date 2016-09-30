#include <stdexcept>
#include <system_error>

#include <sys/uio.h>
#include <dirent.h>

#include "internal/fs/File.h"
#include "internal/fs/FileDescriptor.h"
#include "internal/memory/UserMemory.h"
#include "internal/syscall/fs.h"

#undef getdents64
#undef stat64

namespace syscall {

namespace {
    async::future<ssize_t> _preadwritev2(bool isWrite, std::weak_ptr<process::Process> process, int fd, const std::vector<fs::IoVector>& iov, off64_t offset) {
        if (iov.size() == 0)
            return async::make_ready_future(0);

        fs::OpenFile::Flags flags = {
            .read = !isWrite,
            .write = isWrite
        };

        auto file = std::shared_ptr<process::Process>(process)->fdTable.get(fd, flags);
        if (isWrite)
            return file->write(iov, offset);
        else
            return file->read(iov, offset);
    }

    async::future<std::vector<fs::IoVector>> _getIovs(std::weak_ptr<process::Process> process, memory::vaddr_t iov, size_t iovcnt) {
        if (iovcnt > IOV_MAX)
            throw std::invalid_argument("Too many IO vectors");

        return memory::UserMemory(process, iov).get<iovec>(iovcnt).then([process, iovcnt](auto iov) {
            auto _iov = iov.get();

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
        });
    }

    async::future<ssize_t> _preadwritev(bool isWrite, std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) {
        return _getIovs(process, iov, iovcnt).then([=](auto iov) {
            return _preadwritev2(isWrite, process, fd, iov.get(), offset);
        });
    }

    async::future<ssize_t> _preadwrite(bool isWrite, std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) {
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

async::future<int> stat64(std::weak_ptr<process::Process> process, memory::vaddr_t pathname, memory::vaddr_t buf) {
    return memory::UserMemory(process, pathname).readString().then([](auto pathname) {
        return fs::rootFileSystem->stat(pathname.get());
    }).unwrap().then([process, buf](auto stat) mutable {
        return memory::UserMemory(process, buf).set(stat.get());
    }).unwrap().then([](async::future<void> result) {
        result.get();
        return 0;
    });
}

async::future<int> open(std::weak_ptr<process::Process> process, memory::vaddr_t pathname, int flags, mode_t mode) {
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

    if (flags & O_CREAT) {
        if (mode & ~07777)
            throw std::invalid_argument("Invalid mode");

        openFlags.createOnMissing = true;
        openFlags.mode = mode;
    }

    if (flags & ~(O_ACCMODE | O_CREAT | O_DIRECTORY))
        throw std::invalid_argument("Invalid flags");

    return memory::UserMemory(process, pathname).readString().then([openFlags](auto pathname) {
        return fs::rootFileSystem->open(pathname.get(), openFlags);
    }).unwrap().then([flags, openFlags, process](auto file) {
        auto _file = file.get();

        if (std::dynamic_pointer_cast<fs::Directory>(_file)) {
            if (openFlags.write)
                throw std::system_error(EISDIR, std::system_category(), "Cannot open a directory for writing");
        } else {
            if (flags & O_DIRECTORY)
                throw std::system_error(ENOTDIR, std::system_category(), "O_DIRECTORY set but file isn't a directory");
        }

        return std::shared_ptr<process::Process>(process)->fdTable.insert(std::make_shared<fs::OpenFile>(
            _file,
            fs::OpenFile::Flags{.read = openFlags.read, .write = openFlags.write}
        ));
    });
}

async::future<int> close(std::weak_ptr<process::Process> process, int fd) {
    if (std::shared_ptr<process::Process>(process)->fdTable.erase(fd))
        return async::make_ready_future(0);

    throw std::system_error(EBADF, std::system_category());
}

async::future<int> read(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count) {
    return _preadwrite(false, process, fd, buf, count, fs::CURRENT_OFFSET);
}
async::future<int> readv(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt) {
    return _preadwritev(false, process, fd, iov, iovcnt, fs::CURRENT_OFFSET);
}
async::future<int> pread(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) {
    if (offset < 0)
        throw std::invalid_argument("Invalid offset");
    return _preadwrite(false, process, fd, buf, count, offset);
}
async::future<int> preadv(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) {
    if (offset < 0)
        throw std::invalid_argument("Invalid offset");
    return _preadwritev(false, process, fd, iov, iovcnt, offset);
}

async::future<int> write(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count) {
    return _preadwrite(true, process, fd, buf, count, fs::CURRENT_OFFSET);
}
async::future<int> writev(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt) {
    return _preadwritev(true, process, fd, iov, iovcnt, fs::CURRENT_OFFSET);
}
async::future<int> pwrite(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count, off64_t offset) {
    if (offset < 0)
        throw std::invalid_argument("Invalid offset");
    return _preadwrite(true, process, fd, buf, count, offset);
}
async::future<int> pwritev(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset) {
    if (offset < 0)
        throw std::invalid_argument("Invalid offset");
    return _preadwritev(true, process, fd, iov, iovcnt, offset);
}

async::future<int> getdents64(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t dirp, size_t count) {
    if (dirp & ~-alignof(dirent))
        throw std::system_error(EFAULT, std::system_category(), "Result buffer isn't aligned");

    auto file = std::shared_ptr<process::Process>(process)->fdTable.get(fd, fs::OpenFile::Flags{});
    auto directory = std::dynamic_pointer_cast<fs::Directory>(file);
    if (!directory)
        throw std::system_error(ENOTDIR, std::system_category());

    return directory->getdents(memory::UserMemory(process, dirp), count);
}

async::future<int> fcntl64(std::weak_ptr<process::Process> process, int fd, int cmd, int arg) {
    std::shared_ptr<process::Process>(process)->fdTable.get(fd, fs::OpenFile::Flags{});

    if (cmd == F_SETFD && arg == FD_CLOEXEC)
        return async::make_ready_future(0); // XXX: Ignore for now

    throw std::system_error(ENOSYS, std::system_category());
}

async::future<int> ioctl(std::weak_ptr<process::Process> process, int fd, size_t request, memory::vaddr_t argp) {
    return std::shared_ptr<process::Process>(process)->
        fdTable.get(fd, fs::OpenFile::Flags{})->
            ioctl(request, memory::UserMemory(process, argp));
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
