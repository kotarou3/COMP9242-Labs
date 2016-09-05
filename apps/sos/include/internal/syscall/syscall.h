#pragma once

#include <exception>

#include <stdarg.h>
#include <sys/syscall.h>

#define syscall(...) _syscall(__VA_ARGS__)
#include <boost/thread/future.hpp>
#undef syscall

#include "internal/process/Thread.h"

extern "C" {
    #include <sel4/types.h>
}

namespace syscall {

boost::future<int> handle(process::Thread& thread, long number, size_t argc, seL4_Word* argv);
boost::future<int> handle(process::Process& process, long number, size_t argc, seL4_Word* argv);

int exceptionToErrno(std::exception_ptr e) noexcept;

#define FORWARD_SYSCALL(name, argc)                                                          \
    extern "C" int sys_##name(va_list ap) noexcept {                                         \
        seL4_Word argv[argc];                                                                \
        for (size_t a = 0; a < argc; ++a)                                                    \
            argv[a] = va_arg(ap, seL4_Word);                                                 \
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
