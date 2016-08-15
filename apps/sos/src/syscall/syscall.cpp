#include <errno.h>

#include "internal/syscall/syscall.h"

extern "C" {
    #include <serial/serial.h>
    #include <sos.h>

    #define verbose 5
    #include "internal/sys/debug.h"
    #include "internal/sys/panic.h"
}

namespace syscall {

int handle(process::Thread& thread, long number, size_t argc, seL4_Word* argv) {
    switch (number) {
        case SOS_SYS_SERIAL_WRITE: {
            static struct serial *serial;
            if (!serial)
                serial = serial_init();

            return serial_send(serial, reinterpret_cast<char*>(argv), argc);
        }
    }

    kprintf(0, "Unknown syscall %d\n", number);
    return -ENOSYS;
}

}
