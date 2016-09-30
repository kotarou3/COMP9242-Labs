#pragma once

#include "internal/syscall/syscall.h"

namespace syscall {

async::future<int> brk(std::weak_ptr<process::Process> process, memory::vaddr_t addr);
async::future<int> mmap2(std::weak_ptr<process::Process> process, memory::vaddr_t addr, size_t length, int prot, int flags, int fd, off_t offset);
async::future<int> munmap(std::weak_ptr<process::Process> process, memory::vaddr_t addr, size_t length);

}
