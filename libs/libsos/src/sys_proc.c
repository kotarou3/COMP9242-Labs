#include "syscall.h"

FORWARD_SYSCALL(execve, 1)
FORWARD_SYSCALL(kill, 1)
FORWARD_SYSCALL(waitid, 1)

int sys_procstatus(va_list ap) {
    // ap[0] = process[n], ap[1] = n
    (void)ap;
    // TODO: ls /proc, fill with contents of opened files
    return 0;
}
