#pragma once

#include <sys/types.h>

#include "internal/syscall/syscall.h"

namespace syscall {

async::future<pid_t> gettid(std::weak_ptr<process::Thread> thread);

async::future<int> tkill(std::weak_ptr<process::Process> process, pid_t tid, int signal);
async::future<int> tgkill(std::weak_ptr<process::Process> process, pid_t tgid, pid_t tid, int signal);

async::future<int> exit(std::weak_ptr<process::Thread> thread, int status);

}
