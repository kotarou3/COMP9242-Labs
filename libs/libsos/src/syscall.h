#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <sys/syscall.h>

#include <sel4/sel4.h>
#include <sos.h>

#define FORWARD_SYSCALL(name, argc)                                                  \
    int sys_##name(va_list ap) {                                                     \
        seL4_SetMR(0, SYS_##name);                                                   \
        for (size_t a = 0; a < argc; ++a)                                            \
            seL4_SetMR(a + 1, va_arg(ap, seL4_Word));                                \
                                                                                     \
        seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, argc + 1); \
        seL4_Call(SOS_IPC_EP_CAP, req);                                              \
        return seL4_GetMR(0);                                                        \
    }
