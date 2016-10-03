#include <algorithm>
#include <string>
#include <system_error>

#include <sys/syscall.h>
#include <errno.h>

extern "C" {
    #include <sos.h>

    #include "internal/sys/debug.h"
}

#include "internal/syscall/fs.h"
#include "internal/syscall/mmap.h"
#include "internal/syscall/process.h"
#include "internal/syscall/syscall.h"
#include "internal/syscall/time.h"
#include "internal/syscall/thread.h"

namespace syscall {

namespace {
    using ThreadSyscall = async::future<int> (*)(
        std::weak_ptr<process::Thread>,
        seL4_Word, seL4_Word, seL4_Word, seL4_Word,
        seL4_Word, seL4_Word, seL4_Word, seL4_Word
    );
    using ProcessSyscall = async::future<int> (*)(
        std::weak_ptr<process::Process>,
        seL4_Word, seL4_Word, seL4_Word, seL4_Word,
        seL4_Word, seL4_Word, seL4_Word, seL4_Word
    );

    constexpr ThreadSyscall _getThreadSyscall(long number) {
        #define ADD_SYSCALL(name) if (number == SYS_##name) return reinterpret_cast<ThreadSyscall>(syscall::name)

        // thread
        ADD_SYSCALL(gettid);
        ADD_SYSCALL(exit);

        #undef ADD_SYSCALL
        return nullptr;
    }

    constexpr ProcessSyscall _getProcessSyscall(long number) {
        #define ADD_SYSCALL(name) if (number == SYS_##name) return reinterpret_cast<ProcessSyscall>(syscall::name)

        // fs
        ADD_SYSCALL(stat64);
        ADD_SYSCALL(open);
        ADD_SYSCALL(close);

        ADD_SYSCALL(read);
        ADD_SYSCALL(readv);
        ADD_SYSCALL(pread64);
        ADD_SYSCALL(preadv);

        ADD_SYSCALL(write);
        ADD_SYSCALL(writev);
        ADD_SYSCALL(pwrite64);
        ADD_SYSCALL(pwritev);

        ADD_SYSCALL(getdents64);

        ADD_SYSCALL(fcntl64);
        ADD_SYSCALL(ioctl);

        // mmap
        ADD_SYSCALL(brk);
        ADD_SYSCALL(mmap2);
        ADD_SYSCALL(munmap);

        // process
        ADD_SYSCALL(getpid);
        ADD_SYSCALL(waitid);
        ADD_SYSCALL(wait4);
        ADD_SYSCALL(kill);
        ADD_SYSCALL(exit_group);
        ADD_SYSCALL(process_create);
        ADD_SYSCALL(sos_process_status);

        // time
        ADD_SYSCALL(clock_gettime);
        ADD_SYSCALL(nanosleep);

        // thread
        ADD_SYSCALL(tkill);
        ADD_SYSCALL(tgkill);

        #undef ADD_SYSCALL
        return nullptr;
    }
}

async::future<int> handle(std::weak_ptr<process::Thread> thread, long number, size_t argc, seL4_Word* argv) {
    if (_getThreadSyscall(number)) {
        seL4_Word args[8] = {0};
        std::copy(argv, argv + std::min(argc, 8U), args);

        return _getThreadSyscall(number)(
            thread,
            args[0], args[1], args[2], args[3],
            args[4], args[5], args[6], args[7]
        );
    } else {
        return handle(std::shared_ptr<process::Thread>(thread)->getProcess(), number, argc, argv);
    }
}

async::future<int> handle(std::weak_ptr<process::Process> process, long number, size_t argc, seL4_Word* argv) {
    if (_getProcessSyscall(number)) {
        seL4_Word args[8] = {0};
        std::copy(argv, argv + std::min(argc, 8U), args);

        return _getProcessSyscall(number)(
            process,
            args[0], args[1], args[2], args[3],
            args[4], args[5], args[6], args[7]
        );
    }

    throw std::system_error(ENOSYS, std::system_category(), "Unknown syscall " + std::to_string(number));
}

int exceptionToErrno(std::exception_ptr e) noexcept {
    try {
        std::rethrow_exception(e);
    } catch (const std::bad_alloc& e) {
        kprintf(LOGLEVEL_DEBUG, "Got std::bad_alloc: %s\n", e.what());
        return ENOMEM;
    } catch (const std::bad_weak_ptr& e) {
        kprintf(LOGLEVEL_DEBUG, "Got std::bad_weak_ptr: %s (Process probably got killed while executing a syscall)\n", e.what());
        return ENOMEM;
    } catch (const std::invalid_argument& e) {
        kprintf(LOGLEVEL_DEBUG, "Got std::invalid_argument: %s\n", e.what());
        return EINVAL;
    } catch (const std::system_error& e) {
        if (e.code().category() != std::system_category())
            throw;

        kprintf(LOGLEVEL_DEBUG, "Got std::system_error: %s\n", e.what());
        return e.code().value();
    }

    // Intentionally don't catch (...) since we want to std::terminate() in that
    // case (It should never happen!)
}

}
