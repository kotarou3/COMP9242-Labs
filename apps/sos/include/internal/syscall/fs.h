#pragma once

#include <fcntl.h>

#include "internal/syscall/syscall.h"

namespace syscall {

async::future<int> stat64(std::weak_ptr<process::Process> process, memory::vaddr_t pathname, memory::vaddr_t buf);
async::future<int> open(std::weak_ptr<process::Process> process, memory::vaddr_t pathname, int flags, mode_t mode);
async::future<int> close(std::weak_ptr<process::Process> process, int fd);

async::future<int> read(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count);
async::future<int> readv(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt);
async::future<int> pread(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count, off64_t offset);
async::future<int> preadv(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset);

async::future<int> write(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count);
async::future<int> writev(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt);
async::future<int> pwrite(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t buf, size_t count, off64_t offset);
async::future<int> pwritev(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t iov, size_t iovcnt, off64_t offset);

async::future<int> getdents64(std::weak_ptr<process::Process> process, int fd, memory::vaddr_t dirp, size_t count);

async::future<int> fcntl64(std::weak_ptr<process::Process> process, int fd, int cmd, int arg);
async::future<int> ioctl(std::weak_ptr<process::Process> process, int fd, size_t request, memory::vaddr_t argp);

}
