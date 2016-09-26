#pragma once

#include <fcntl.h>

#include "internal/syscall/syscall.h"

namespace syscall {

async::future<int> stat64(process::Process& process, memory::vaddr_t pathname, memory::vaddr_t buf);
async::future<int> open(process::Process& process, memory::vaddr_t pathname, int flags, mode_t mode);
async::future<int> close(process::Process& process, int fd);

async::future<int> read(process::Process& process, int fd, memory::vaddr_t buf, size_t count);
async::future<int> readv(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt);
async::future<int> pread(process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset);
async::future<int> preadv(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset);

async::future<int> write(process::Process& process, int fd, memory::vaddr_t buf, size_t count);
async::future<int> writev(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt);
async::future<int> pwrite(process::Process& process, int fd, memory::vaddr_t buf, size_t count, off64_t offset);
async::future<int> pwritev(process::Process& process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset);

async::future<int> getdents64(process::Process& process, int fd, memory::vaddr_t dirp, size_t count);

async::future<int> fcntl64(process::Process& process, int fd, int cmd, int arg);
async::future<int> ioctl(process::Process& process, int fd, size_t request, memory::vaddr_t argp);

}
