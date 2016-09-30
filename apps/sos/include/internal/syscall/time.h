#pragma once

#include "internal/syscall/syscall.h"

namespace syscall {

async::future<int> clock_gettime(std::weak_ptr<process::Process> process, clockid_t clk_id, memory::vaddr_t tp);
async::future<int> nanosleep(std::weak_ptr<process::Process> process, memory::vaddr_t req, memory::vaddr_t rem);

}
