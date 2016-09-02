#include <algorithm>
#include <sys/syscall.h>
#include <errno.h>

#include "internal/syscall/fs.h"
#include "internal/syscall/mmap.h"
#include "internal/syscall/syscall.h"
#include "internal/syscall/time.h"

extern "C" {
    #include "internal/sys/debug.h"
}

namespace syscall {

namespace {
    using ThreadSyscall = boost::future<int> (*)(
        process::Thread&,
	    seL4_Word, seL4_Word, seL4_Word, seL4_Word,
	    seL4_Word, seL4_Word, seL4_Word, seL4_Word
    );
    using ProcessSyscall = boost::future<int> (*)(
        process::Process&,
	    seL4_Word, seL4_Word, seL4_Word, seL4_Word,
	    seL4_Word, seL4_Word, seL4_Word, seL4_Word
    );

    constexpr ThreadSyscall _getThreadSyscall(long /*number*/) {
        #define ADD_SYSCALL(name) if (number == SYS_##name) return reinterpret_cast<ThreadSyscall>(syscall::name)
        #undef ADD_SYSCALL
	    return nullptr;
    }

    constexpr ProcessSyscall _getProcessSyscall(long number) {
        #define ADD_SYSCALL(name) if (number == SYS_##name) return reinterpret_cast<ProcessSyscall>(syscall::name)

	    // fs
        ADD_SYSCALL(open);
        ADD_SYSCALL(close);
        ADD_SYSCALL(stat);

        ADD_SYSCALL(read);
        ADD_SYSCALL(readv);
        ADD_SYSCALL(pread64);
        ADD_SYSCALL(preadv);

        ADD_SYSCALL(write);
        ADD_SYSCALL(writev);
        ADD_SYSCALL(pwrite64);
        ADD_SYSCALL(pwritev);

        ADD_SYSCALL(ioctl);

	    // mmap
	    ADD_SYSCALL(brk);
	    ADD_SYSCALL(mmap2);
	    ADD_SYSCALL(munmap);

	    // time
	    ADD_SYSCALL(clock_gettime);
	    ADD_SYSCALL(nanosleep);

	    #undef ADD_SYSCALL
	    return nullptr;
    }
}

boost::future<int> handle(process::Thread& thread, long number, size_t argc, seL4_Word* argv) noexcept {
    if (_getThreadSyscall(number)) {
        seL4_Word args[8] = {0};
        std::copy(argv, argv + std::min(argc, 8U), args);

        return _getThreadSyscall(number)(
            thread,
            args[0], args[1], args[2], args[3],
            args[4], args[5], args[6], args[7]
        );
    } else {
        return handle(thread.getProcess(), number, argc, argv);
    }
}

boost::future<int> handle(process::Process& process, long number, size_t argc, seL4_Word* argv) noexcept {
    if (_getProcessSyscall(number)) {
        seL4_Word args[8] = {0};
        std::copy(argv, argv + std::min(argc, 8U), args);

        return _getProcessSyscall(number)(
            process,
            args[0], args[1], args[2], args[3],
            args[4], args[5], args[6], args[7]
        );
    } else {
        kprintf(0, "Unknown syscall %d\n", number);
        return _returnNow(-ENOSYS);
    }
}

}
