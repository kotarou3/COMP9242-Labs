#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/syscall.h>

#include <sel4/sel4.h>
#include <sos.h>

int sys_brk(va_list ap)
{
    seL4_SetMR(0, SYS_brk);
    seL4_SetMR(1, va_arg(ap, seL4_Word));

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 2);
    seL4_Call(SOS_IPC_EP_CAP, req);
    return seL4_GetMR(0);
}

int sys_mmap2(va_list ap)
{
    seL4_SetMR(0, SYS_mmap2);
    seL4_SetMR(1, va_arg(ap, seL4_Word));
    seL4_SetMR(2, va_arg(ap, seL4_Word));
    seL4_SetMR(3, va_arg(ap, seL4_Word));
    seL4_SetMR(4, va_arg(ap, seL4_Word));
    seL4_SetMR(5, va_arg(ap, seL4_Word));
    seL4_SetMR(6, va_arg(ap, seL4_Word));

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 7);
    seL4_Call(SOS_IPC_EP_CAP, req);
    return seL4_GetMR(0);
}

int sys_munmap(va_list ap)
{
    seL4_SetMR(0, SYS_munmap);
    seL4_SetMR(1, va_arg(ap, seL4_Word));
    seL4_SetMR(2, va_arg(ap, seL4_Word));

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 3);
    seL4_Call(SOS_IPC_EP_CAP, req);
    return seL4_GetMR(0);
}

int sys_mremap()
{
    assert(!"not implemented");
    __builtin_unreachable();
}

int sys_madvise()
{
    // free() needs this
    return -ENOSYS;
}
