#pragma once

#include <fcntl.h>

#include "internal/syscall/syscall.h"

namespace syscall {

boost::future<int> open(process::Process& process, memory::vaddr_t pathname, int flags, mode_t mode);
boost::future<int> close(process::Process& process, int fd);
boost::future<int> stat(process::Process& process, memory::vaddr_t pathname, memory::vaddr_t result);


boost::future<int> read(process::Process& process, int fd, memory::vaddr_t buf, size_t count);
boost::future<int> readv(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt);
boost::future<int> pread(process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset);
boost::future<int> preadv(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset);

boost::future<int> write(process::Process& process, int fd, memory::vaddr_t buf, size_t count);
boost::future<int> writev(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt);
boost::future<int> pwrite(process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset);
boost::future<int> pwritev(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset);

boost::future<int> ioctl(process::Process& process, int fd, size_t request, size_t arg);

}
