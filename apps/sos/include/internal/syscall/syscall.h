#pragma once

#include "internal/process/Thread.h"

extern "C" {
    #include <sel4/types.h>
}

namespace syscall {

int handle(process::Thread& thread, long number, size_t argc, seL4_Word* argv) noexcept;

}
