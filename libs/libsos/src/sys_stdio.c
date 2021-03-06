#include "syscall.h"

FORWARD_SYSCALL(stat64, 2);
FORWARD_SYSCALL(open, 3);
FORWARD_SYSCALL(close, 1);

FORWARD_SYSCALL(read, 3);
FORWARD_SYSCALL(readv, 3);
FORWARD_SYSCALL(pread64, 6);
FORWARD_SYSCALL(preadv, 6);

FORWARD_SYSCALL(write, 3);
FORWARD_SYSCALL(writev, 3);
FORWARD_SYSCALL(pwrite64, 6);
FORWARD_SYSCALL(pwritev, 6);

FORWARD_SYSCALL(getdents64, 3);

FORWARD_SYSCALL(fcntl64, 3);
FORWARD_SYSCALL(ioctl, 3);
