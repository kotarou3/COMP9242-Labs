#pragma once

#include "internal/process/Thread.h"
#include "internal/syscall/syscall.h"

namespace syscall {

boost::future<int> brk(process::Process& process, memory::vaddr_t addr) noexcept;
boost::future<int> mmap2(process::Process& process, memory::vaddr_t addr, size_t length, int prot, int flags, int fd, off_t offset) noexcept;
boost::future<int> munmap(process::Process& process, memory::vaddr_t addr, size_t length) noexcept;

}
