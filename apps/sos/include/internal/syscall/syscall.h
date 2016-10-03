#pragma once

#include <exception>
#include <memory>

#include <stdarg.h>
#include <sys/syscall.h>

extern "C" {
    #include <sel4/types.h>
}

#include "internal/async.h"
#include "internal/process/Thread.h"

namespace syscall {

async::future<int> handle(std::weak_ptr<process::Thread> thread, long number, size_t argc, seL4_Word* argv);
async::future<int> handle(std::weak_ptr<process::Process> process, long number, size_t argc, seL4_Word* argv);

int exceptionToErrno(std::exception_ptr e) noexcept;

#define FORWARD_SYSCALL(name, argc)                                                          \
    extern "C" int sys_##name(va_list ap) noexcept {                                         \
        seL4_Word argv[argc];                                                                \
        _Pragma("GCC diagnostic ignored \"-Wtype-limits\"");                                 \
        for (size_t a = 0; a < argc; ++a)                                                    \
            argv[a] = va_arg(ap, seL4_Word);                                                 \
        _Pragma("GCC diagnostic pop");                                                       \
                                                                                             \
        try {                                                                                \
            auto result = syscall::handle(process::getSosProcess(), SYS_##name, argc, argv); \
            assert(result.is_ready());                                                       \
            return result.get();                                                             \
        } catch (...) {                                                                      \
            return -syscall::exceptionToErrno(std::current_exception());                     \
        }                                                                                    \
    }

}
