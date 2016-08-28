#pragma once

#include "internal/syscall/syscall.h"

namespace syscall {

boost::future<int> clock_gettime(process::Process& process, clockid_t clk_id, memory::vaddr_t tp) noexcept;
boost::future<int> nanosleep(process::Process& process, memory::vaddr_t req, memory::vaddr_t rem) noexcept;

}
