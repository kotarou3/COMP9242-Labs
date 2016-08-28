/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <string.h>

#include <sel4/sel4.h>
#include <sos.h>
#include <unistd.h>
#include <utils/arith.h>

int sos_sys_open(const char *path, fmode_t mode) {
    (void)path;
    (void)mode;
    assert(!"You need to implement this");
    __builtin_unreachable();
}

int sos_sys_read(int file, char *buf, size_t nbyte) {
    (void)file;
    (void)buf;
    (void)nbyte;
    assert(!"You need to implement this");
    __builtin_unreachable();
}

int sos_sys_write(int file, const char *buf, size_t nbyte) {
    // Hardcoded with stdin/err only until full fs syscalls are implemented
    if (file != STDOUT_FILENO && file != STDERR_FILENO)
        assert(!"You need to implement this");

    // No shared memory yet, so we just use the IPC buffer
    nbyte = MIN(nbyte, (size_t)seL4_MsgMaxLength - 1);
    memcpy(&seL4_GetIPCBuffer()->msg[1], buf, nbyte);

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, nbyte + 1);
    seL4_SetMR(0, SOS_SYS_SERIAL_WRITE);
    seL4_Call(SOS_IPC_EP_CAP, req);

    return seL4_GetMR(0);
}

void sos_sys_usleep(int msec) {
    (void)msec;
    assert(!"You need to implement this");
}

int64_t sos_sys_time_stamp(void) {
    assert(!"You need to implement this");
    __builtin_unreachable();
}
