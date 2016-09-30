#pragma once

#include <memory>

#include "internal/async.h"
#include "internal/process/Thread.h"

namespace elf {

async::future<memory::vaddr_t> load(std::shared_ptr<process::Process> process, uint8_t* file);

}
