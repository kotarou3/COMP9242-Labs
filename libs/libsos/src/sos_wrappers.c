#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "sos.h"

static int _unimplemented(void) {
    fprintf(stderr, "system call not implemented\n");
    errno = ENOSYS;
    return -1;
}

int sos_sys_open(const char* path, fmode_t mode) {
    return open(path, mode);
}

int sos_sys_close(int file) {
    return close(file);
}

int sos_sys_read(int file, char *buf, size_t nbyte) {
    return read(file, buf, nbyte);
}

int sos_sys_write(int file, const char *buf, size_t nbyte) {
    return write(file, buf, nbyte);
}

int sos_getdirent(int pos, char *name, size_t nbyte) {
    (void)pos;
    (void)name;
    (void)nbyte;
    return _unimplemented();
}

int sos_stat(const char *path, sos_stat_t *buf) {
    (void)path;
    (void)buf;
    return _unimplemented();
}

pid_t sos_process_create(const char *path) {
    (void)path;
    return _unimplemented();
}

int sos_process_delete(pid_t pid) {
    (void)pid;
    return _unimplemented();
}

pid_t sos_my_id(void) {
    return getpid();
}

int sos_process_status(sos_process_t *processes, unsigned max) {
    (void)processes;
    (void)max;
    return _unimplemented();
}

pid_t sos_process_wait(pid_t pid) {
    (void)pid;
    return _unimplemented();
}

int64_t sos_sys_time_stamp(void) {
    struct timespec timespec;
    if (!clock_gettime(CLOCK_REALTIME, &timespec))
        return -1;

    return (int64_t)timespec.tv_sec * 1000000 + (int64_t)timespec.tv_nsec / 1000;
}

void sos_sys_usleep(int msec) {
    struct timespec timespec;
    timespec.tv_sec = msec / 1000000;
    timespec.tv_nsec = (msec - timespec.tv_sec * 1000000) * 1000;
    nanosleep(&timespec, NULL);
}

int sos_share_vm(void *adr, size_t size, int writable) {
    (void)adr;
    (void)size;
    (void)writable;
    return _unimplemented();
}
