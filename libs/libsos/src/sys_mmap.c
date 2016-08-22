#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/syscall.h>

#include <sel4/sel4.h>
#include <sos.h>

int sys_brk(va_list ap)
{
    void *addr = va_arg(ap, void*);

    seL4_SetMR(0, SYS_brk);
    seL4_SetMR(1, addr);

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 2);
    seL4_Call(SOS_IPC_EP_CAP, req);
    return seL4_GetMR(0);
}

int sys_mmap2(va_list ap)
{
    void *addr = va_arg(ap, void*);
    size_t length = va_arg(ap, size_t);
    int prot = va_arg(ap, int);
    int flags = va_arg(ap, int);
    int fd = va_arg(ap, int);
    off_t offset = va_arg(ap, off_t);

    seL4_SetMR(0, SYS_mmap2);
    seL4_SetMR(1, addr);
    seL4_SetMR(2, length);
    seL4_SetMR(3, prot);
    seL4_SetMR(4, flags);
    seL4_SetMR(5, fd);
    seL4_SetMR(6, offset);

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 7);
    seL4_Call(SOS_IPC_EP_CAP, req);
    return seL4_GetMR(0);
}

int sys_munmap(va_list ap)
{
    void *addr = va_arg(ap, void*);
    size_t length = va_arg(ap, size_t);

    seL4_SetMR(0, SYS_munmap);
    seL4_SetMR(1, addr);
    seL4_SetMR(2, length);

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 7);
    seL4_Call(SOS_IPC_EP_CAP, req);
    return seL4_GetMR(0);
}

int sys_mremap(va_list ap)
{
    assert(!"not implemented");
    return -ENOMEM;
}

int sys_madvise(va_list ap)
{
    // free() needs this
    return -ENOSYS;
}
