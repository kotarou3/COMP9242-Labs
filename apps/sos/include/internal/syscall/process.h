#pragma once

#include <sys/types.h>
#include <sys/wait.h>

#include "internal/syscall/syscall.h"

namespace syscall {

async::future<pid_t> getpid(std::weak_ptr<process::Process> process);

async::future<int> waitid(std::weak_ptr<process::Process> process, idtype_t idtype, id_t id, memory::vaddr_t infop, int options);
async::future<pid_t> wait4(std::weak_ptr<process::Process> process, pid_t pid, memory::vaddr_t status, int options, memory::vaddr_t rusage);

async::future<int> kill(std::weak_ptr<process::Process> process, pid_t pid, int signal);

async::future<int> exit_group(std::weak_ptr<process::Process> process, int status);

async::future<pid_t> process_create(std::weak_ptr<process::Process> process, memory::vaddr_t filename);

async::future<int> sos_process_status(std::weak_ptr<process::Process> process, memory::vaddr_t processes, unsigned max);

}
