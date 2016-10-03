#pragma once

#include <limits>
#include <unordered_map>

#include <sys/types.h>

extern "C" {
    #include <sel4/types.h>
}

#include "internal/async.h"
#include "internal/process/Thread.h"

namespace process {

// Since each thread will need their own unique badged endpoint, and the IPC
// endpoint for SOS is shared with the IRQ endpoint, we can't use the full
// space of the badge as a thread ID. But since IRQ endpoints always set the
// MSB, we can use the remaining bits as a thread ID

constexpr const pid_t MIN_TID = 1;
constexpr const pid_t MAX_TID = (1 << (seL4_BadgeBits - 1)) - 1;

class ThreadTable {
    public:
        pid_t insert(std::shared_ptr<Thread> thread);
        bool erase(pid_t tid) noexcept;

        std::shared_ptr<Thread> get(pid_t tid) const;

        auto size() const {return _table.size();}
        auto begin() const {return _table.begin();}
        auto end() const {return _table.end();}

        static ThreadTable& get() noexcept;

    private:
        pid_t _nextFree = MIN_TID;
        std::unordered_map<pid_t, std::shared_ptr<Thread>> _table;
};

}
