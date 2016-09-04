#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#undef st_ctime
#undef st_atime

#include "sos.h"

#define UMASK 0002

static int _unimplemented(void) {
    fprintf(stderr, "system call not implemented\n");
    errno = ENOSYS;
    return -1;
}

static long posix_to_sos_time(struct timespec time) {
    // should convert time since epoch to ms since boot.
    // for now, assume epoch = boot time
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

static fmode_t posix_to_sos_fmode(mode_t mode) {
    // TODO
    return mode;
}

int sos_sys_open(const char* path, int flags) {
    return open(path, flags);
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
    static DIR *dirp;
    static int curPos;

    if (pos < 0) {
        errno = EINVAL;
        return -1;
    } else if (!dirp) {
        dirp = opendir(".");
        curPos = 0;
    } else if (pos < curPos) {
        closedir(dirp);
        dirp = opendir(".");
        curPos = 0;
    }

    if (!dirp)
        return -1;

    while (1) {
        errno = 0;
        struct dirent *dp = readdir(dirp);
        if (dp) {
            ++curPos;

            if (curPos - 1 == pos) {
                strncpy(name, dp->d_name, nbyte - 1);
                name[nbyte - 1] = 0;
                return strlen(name);
            }
        } else {
            int prevErrno = errno;
            closedir(dirp);
            dirp = NULL;

            errno = prevErrno;
            return errno ? -1 : 0;
        }
    }
}

int sos_stat(const char *path, sos_stat_t *buf) {
    struct stat s;
    int err = stat(path, &s);
    if (!err) {
        buf->st_type = S_ISREG(s.st_mode) ? ST_FILE : ST_SPECIAL;
        buf->st_fmode = posix_to_sos_fmode(s.st_mode);
        buf->st_size = s.st_size;
        buf->st_ctime = posix_to_sos_time(s.st_ctim);
        buf->st_atime = posix_to_sos_time(s.st_atim);
    }
    return err;
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
