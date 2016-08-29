#include <assert.h>
#include <errno.h>

#include "syscall.h"

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

FORWARD_SYSCALL(brk, 1);
FORWARD_SYSCALL(mmap2, 6);
FORWARD_SYSCALL(munmap, 2);
