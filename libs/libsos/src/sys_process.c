#include "syscall.h"

FORWARD_SYSCALL(getpid, 0);
FORWARD_SYSCALL(waitid, 4);
FORWARD_SYSCALL(wait4, 4);
FORWARD_SYSCALL(kill, 2);
FORWARD_SYSCALL(exit_group, 1);
