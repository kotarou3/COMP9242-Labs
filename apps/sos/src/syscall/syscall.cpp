#include <algorithm>
#include <sys/syscall.h>
#include <errno.h>

#include "internal/syscall/mmap.h"
#include "internal/syscall/syscall.h"

extern "C" {
    #include <serial/serial.h>
    #include <sos.h>

    #define verbose 5
    #include "internal/sys/debug.h"
    #include "internal/sys/panic.h"
}

namespace syscall {

namespace {
    using Syscall = int (*)(
        process::Process& process, // Thread-specific syscalls not supported yet
	    seL4_Word, seL4_Word, seL4_Word, seL4_Word,
	    seL4_Word, seL4_Word, seL4_Word, seL4_Word
    );

    constexpr Syscall getSyscall(long number) {
        #define ADD_SYSCALL(name) if (number == SYS_##name) return reinterpret_cast<Syscall>(syscall::name)

	    ADD_SYSCALL(brk);
	    ADD_SYSCALL(mmap2);
	    ADD_SYSCALL(munmap);

	    return nullptr;
    }
}

int handle(process::Thread& thread, long number, size_t argc, seL4_Word* argv) noexcept {
    if (number == SOS_SYS_SERIAL_WRITE) {
        static struct serial *serial;
        if (!serial)
            serial = serial_init();

        return serial_send(serial, reinterpret_cast<char*>(argv), argc);
    } else if (getSyscall(number)) {
        seL4_Word args[8] = {0};
        std::copy(argv, argv + std::min(argc, 8U), args);

        return getSyscall(number)(
            thread.getProcess(),
            args[0], args[1], args[2], args[3],
            args[4], args[5], args[6], args[7]
        );
    } else {
        kprintf(0, "Unknown syscall %d\n", number);
        return -ENOSYS;
    }
}

}
