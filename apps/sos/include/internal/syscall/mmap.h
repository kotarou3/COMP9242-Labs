#pragma once

#include "internal/process/Thread.h"

namespace syscall {

int brk(process::Process& process, memory::vaddr_t addr) noexcept;
int mmap2(process::Process& process, memory::vaddr_t addr, size_t length, int prot, int flags, int fd, off_t offset) noexcept;
int munmap(process::Process& process, memory::vaddr_t addr, size_t length) noexcept;

}
