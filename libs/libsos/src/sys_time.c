#include <stdarg.h>
#include <sys/syscall.h>

#include <sel4/sel4.h>
#include <sos.h>

int sys_clock_gettime(va_list ap) {
    seL4_SetMR(0, SYS_clock_gettime);
    seL4_SetMR(1, va_arg(ap, seL4_Word));
    seL4_SetMR(2, va_arg(ap, seL4_Word));

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 3);
    seL4_Call(SOS_IPC_EP_CAP, req);
    return seL4_GetMR(0);
}

int sys_nanosleep(va_list ap) {
    seL4_SetMR(0, SYS_nanosleep);
    seL4_SetMR(1, va_arg(ap, seL4_Word));
    seL4_SetMR(2, va_arg(ap, seL4_Word));

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 3);
    seL4_Call(SOS_IPC_EP_CAP, req);
    return seL4_GetMR(0);
}
