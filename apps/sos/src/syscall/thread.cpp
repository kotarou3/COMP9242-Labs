#include <stdexcept>
#include <system_error>

#include <signal.h>

#include "internal/process/Table.h"
#include "internal/syscall/thread.h"

namespace syscall {

async::future<pid_t> gettid(std::weak_ptr<process::Thread> thread) {
    return async::make_ready_future(std::shared_ptr<process::Thread>(thread)->getTid());
}

async::future<int> tkill(std::weak_ptr<process::Process> process, pid_t tid, int signal) {
    // XXX: Assumes single-threaded processes where pid = tid
    return syscall::tgkill(process, tid, tid, signal);
}

async::future<int> tgkill(std::weak_ptr<process::Process> process, pid_t tgid, pid_t tid, int signal) {
    // XXX: Assumes single-threaded processes where pid = tid
    if (tgid != tid)
        throw std::system_error(ESRCH, std::system_category());

    if (signal != 0 && signal != SIGKILL)
        throw std::invalid_argument("Only SIGKILL is currently supported");

    if (tgid != std::shared_ptr<process::Process>(process)->getPid()) {
        if (tgid == process::getSosProcess()->getPid())
            throw std::system_error(EPERM, std::system_category(), "Can't kill SOS");
        if (tgid == process::MIN_TID)
            throw std::system_error(EPERM, std::system_category(), "Can't kill init");
    }

    if (tgid == process::getSosProcess()->getPid()) {
        if (signal != 0)
            _exit(1);
    } else {
        auto thread = process::ThreadTable::get().get(tid);
        if (signal != 0)
            thread->kill();
    }

    return async::make_ready_future(0);
}

async::future<int> exit(std::weak_ptr<process::Thread> thread, int /*status*/) {
    // TODO: Exit codes are not supported
    std::shared_ptr<process::Thread>(thread)->kill();

    return async::make_exceptional_future<int>(std::logic_error("Returned from syscall::exit()???"));
}

}

extern "C" int sys_gettid() {
    return 0;
}

extern "C" void sys_exit(va_list ap) {
    // We don't support multithreading, so just exit the entire process
    _exit(va_arg(ap, int));
}

extern "C" int sys_rt_sigprocmask(va_list /*ap*/) {
    // XXX: abort() unmasks SIGABRT, but we don't support masking, so pretend
    // it was successful
    return 0;
}

extern "C" pid_t sys_set_tid_address(va_list /*ap*/) {
    // XXX: Dummy function - but muslc needs it
    return sys_gettid();
}

FORWARD_SYSCALL(tkill, 2);
FORWARD_SYSCALL(tgkill, 3);
