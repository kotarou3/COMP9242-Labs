#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sos.h"

__attribute__((__constructor__))
static void open_console(void) {
    // The spec is very picky in what is open at process start
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // XXX: dup2() not implemented, so this will have to do
    assert(open("console", O_RDONLY) == STDIN_FILENO);
    assert(open("console", O_WRONLY) == STDOUT_FILENO);
    assert(open("console", O_WRONLY) == STDERR_FILENO);
    close(STDIN_FILENO);
}

static long posix_to_sos_time(struct timespec time) {
    // should convert time since epoch to ms since boot.
    // for now, assume epoch = boot time
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

static fmode_t posix_to_sos_fmode(mode_t mode) {
    // No idea what user we end up using on the nfs, so assume all files
    // are owned by us
    fmode_t result = 0;
    if (mode & S_IXUSR)
        result |= FM_EXEC;
    if (mode & S_IWUSR)
        result |= FM_WRITE;
    if (mode & S_IRUSR)
        result |= FM_READ;
    return result;
}

int sos_sys_open(const char* path, int flags) {
    return open(path, flags | O_CREAT, 0666);
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
    seL4_SetMR(0, SYS_process_create);
    seL4_SetMR(1, (seL4_Word)path);

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 2);
    seL4_Call(SOS_IPC_EP_CAP, req);

    pid_t pid = (pid_t)seL4_GetMR(0);
    if (pid < 0) {
        errno = -pid;
        return -1;
    } else {
        return pid;
    }
}

int sos_process_delete(pid_t pid) {
    return kill(pid, SIGKILL);
}

pid_t sos_my_id(void) {
    return getpid();
}

int sos_process_status(sos_process_t *processes, unsigned max) {
    seL4_SetMR(0, SYS_sos_process_status);
    seL4_SetMR(1, (seL4_Word)processes);
    seL4_SetMR(2, (seL4_Word)max);

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 3);
    seL4_Call(SOS_IPC_EP_CAP, req);

    int result = (int)seL4_GetMR(0);
    if (result < 0) {
        errno = -result;
        return -1;
    } else {
        return result;
    }
}

pid_t sos_process_wait(pid_t pid) {
    return waitpid(pid, NULL, 0);
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
    seL4_SetMR(0, SYS_sos_share_vm);
    seL4_SetMR(1, (seL4_Word)adr);
    seL4_SetMR(2, (seL4_Word)size);
    seL4_SetMR(3, (seL4_Word)writable);

    seL4_MessageInfo_t req = seL4_MessageInfo_new(seL4_NoFault, 0, 0, 4);
    seL4_Call(SOS_IPC_EP_CAP, req);

    int result = (int)seL4_GetMR(0);
    if (result == -ERESTART)
        return sos_share_vm(adr, size, writable);

    if (result < 0) {
        errno = -result;
        return -1;
    } else {
        return result;
    }
}
