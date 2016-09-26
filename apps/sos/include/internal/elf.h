#pragma once

#include "internal/async.h"
#include "internal/process/Thread.h"

namespace elf {

async::future<memory::vaddr_t> load(process::Process& process, uint8_t* file);

}
