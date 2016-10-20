#pragma once

#include <memory>
#include <string>

#include "internal/async.h"
#include "internal/process/Process.h"

namespace elf {

async::future<memory::vaddr_t> load(std::shared_ptr<process::Process> process, const std::string& pathname);

}
