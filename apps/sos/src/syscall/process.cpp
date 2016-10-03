#include <algorithm>
#include <limits>
#include <stdexcept>
#include <system_error>

extern "C" {
    #include <cpio/cpio.h>
    #include <sos.h>

    #include "internal/sys/debug.h"
}

#include "internal/elf.h"
#include "internal/globals.h"
#include "internal/fs/File.h"
#include "internal/memory/UserMemory.h"
#include "internal/process/Table.h"
#include "internal/syscall/process.h"
#include "internal/syscall/thread.h"

extern char _cpio_archive[];

namespace syscall {

async::future<pid_t> getpid(std::weak_ptr<process::Process> process) {
    return async::make_ready_future(std::shared_ptr<process::Process>(process)->getPid());
}

async::future<int> waitid(std::weak_ptr<process::Process> process, idtype_t idtype, id_t id, memory::vaddr_t infop, int options) {
    if (options != 0)
        throw std::system_error(ENOSYS, std::system_category(), "Options are not supported");
    if (idtype != P_PID && idtype != P_ALL)
        throw std::invalid_argument("Invalid id type");

    auto promise = std::make_shared<async::promise<int>>();
    std::shared_ptr<process::Process>(process)->onChildExit([=](auto process) noexcept {
        pid_t pid = process->getPid();
        if (idtype != P_ALL && !(idtype == P_PID && pid == static_cast<pid_t>(id)))
            return false;

        siginfo_t info;
        info.si_pid = pid;
        info.si_uid = 0; // XXX: Dummy value
        info.si_signo = SIGCHLD;

        // TODO: Reporting exit status not supported
        info.si_status = 0;
        info.si_code = CLD_EXITED;

        try {
            memory::UserMemory(process, infop).set(info).then([=](async::future<void> result) noexcept {
                try {
                    result.get();
                    promise->set_value(0);
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            });
            return true;
        } catch (...) {
            promise->set_exception(std::current_exception());
            return false;
        }
    });

    return promise->get_future();
}

async::future<pid_t> wait4(std::weak_ptr<process::Process> process, pid_t pid, memory::vaddr_t status, int options, memory::vaddr_t rusage) {
    if (pid < -1 || pid == 0)
        throw std::system_error(ENOSYS, std::system_category(), "Process groups not supported");
    if (options != 0)
        throw std::system_error(ENOSYS, std::system_category(), "Options are not supported");
    if (rusage != 0)
        throw std::system_error(ENOSYS, std::system_category(), "Resource usage reporting is not supported");

    auto promise = std::make_shared<async::promise<int>>();
    std::shared_ptr<process::Process>(process)->onChildExit([=](auto process) noexcept {
        pid_t childPid = process->getPid();
        if (pid != -1 && pid != childPid)
            return false;

        try {
            // TODO: Reporting exit status not supported
            memory::UserMemory(process, status).set<int>(0).then([=](async::future<void> result) noexcept {
                try {
                    result.get();
                    promise->set_value(childPid);
                } catch (...) {
                    promise->set_value(childPid);
                }
            });
        } catch (...) {
            promise->set_value(childPid);
        }

        return true;
    });

    return promise->get_future();
}

async::future<int> kill(std::weak_ptr<process::Process> process, pid_t pid, int signal) {
    if (pid < 1)
        throw std::system_error(ENOSYS, std::system_category(), "Killing multiple processes at once is not supported");

    // TODO: Only supports single-threaded processes
    return syscall::tgkill(process, pid, pid, signal);
}

async::future<int> exit_group(std::weak_ptr<process::Process> process, int /*status*/) {
    // TODO: Exit codes are not supported
    for (auto thread : std::shared_ptr<process::Process>(process)->getThreads()) {
        auto _thread = thread.lock();
        assert(_thread);
        _thread->kill();
    }

    return async::make_exceptional_future<int>(std::logic_error("Returned from syscall::exit_group()???"));
}

async::future<pid_t> process_create(std::weak_ptr<process::Process> process, memory::vaddr_t filename) {
    // exec-like in that fds are copied from parent to child
    auto newProcess = process::Process::create(std::shared_ptr<process::Process>(process));
    return memory::UserMemory(process, filename).readString().then([=](auto filename) {
        newProcess->filename = std::move(filename.get());

        unsigned long elf_size;
        uint8_t* elf_base = (uint8_t*)cpio_get_file(_cpio_archive, newProcess->filename.c_str(), &elf_size);
        if (!elf_base)
            throw std::system_error(ENOENT, std::system_category(), "Unable to locate cpio header");

        return elf::load(newProcess, elf_base);
    }).unwrap().then([=](auto ep) {
        newProcess->fdTable = std::shared_ptr<process::Process>(process)->fdTable;

        auto newThread = process::Thread::create(newProcess);
        pid_t tid = process::ThreadTable::get().insert(newThread);
        try {
            return newThread->start(tid, getIpcEndpoint(), tid, ep.get()).then([=](async::future<void> result) {
                try {
                    result.get();
                    return tid;
                } catch (...) {
                    process::ThreadTable::get().erase(tid);
                    throw;
                }
            });
        } catch (...) {
            process::ThreadTable::get().erase(tid);
            throw;
        }
    });
}

async::future<int> sos_process_status(std::weak_ptr<process::Process> process, memory::vaddr_t processes, unsigned max) {
    max = std::min(max, static_cast<unsigned>(std::numeric_limits<int>::max()));
    return memory::UserMemory(process, processes)
        .mapIn<sos_process_t>(max, memory::Attributes{.read = false, .write = true})
        .then([=](auto map) {
            auto _map = std::move(map.get());

            size_t n = 0;
            for (const auto& thread : process::ThreadTable::get()) {
                if (n >= max)
                    break;

                // XXX: Assumes single-threaded processes and pid = tid
                auto process = thread.second->getProcess();
                if (process->isZombie())
                    continue;

                using namespace std::chrono;
                _map.first[n].pid = thread.first;
                _map.first[n].size = process->pageDirectory.countPages();
                _map.first[n].stime = duration_cast<milliseconds>(thread.second->getStartTime().time_since_epoch()).count();
                _map.first[n].command[process->filename.copy(_map.first[n].command, N_NAME - 1)] = 0;
                ++n;
            }

            return static_cast<int>(n);
        });
}

}

extern "C" void sys_exit_group(va_list /*ap*/) {
    kprintf(LOGLEVEL_EMERG, "SOS aborting\n");

    kprintf(LOGLEVEL_EMERG, "Waiting for debugger by spinning...\n");
    while (1)
        ;
}

FORWARD_SYSCALL(getpid, 0);
FORWARD_SYSCALL(waitid, 4);
FORWARD_SYSCALL(wait4, 4);
FORWARD_SYSCALL(kill, 2);
