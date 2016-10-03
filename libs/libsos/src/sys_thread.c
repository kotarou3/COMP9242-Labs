#define _GNU_SOURCE
#include <unistd.h>
#include "syscall.h"

int sys_rt_sigprocmask() {
    // XXX: abort() unmasks SIGABRT, but we don't support masking, so pretend
    // it was successful
    return 0;
}

pid_t sys_set_tid_address() {
    // XXX: Dummy function - but muslc needs it
    return syscall(SYS_gettid);
}

FORWARD_SYSCALL(gettid, 0);
FORWARD_SYSCALL(tkill, 2);
FORWARD_SYSCALL(tgkill, 3);
FORWARD_SYSCALL(exit, 1);
