#pragma once

#include <memory>

extern "C" {
    #include <cspace/cspace.h>
    #include <sel4/types.h>
}

#include "internal/async.h"
#include "internal/memory/Mappings.h"
#include "internal/timer/timer.h"
#include "internal/Capability.h"

namespace process {

class Process;
class Thread : public std::enable_shared_from_this<Thread> {
    public:
        ~Thread();

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        Thread(Thread&& other) = delete;
        Thread& operator=(Thread&& other) = delete;

        async::future<void> start(
            pid_t tid,
            const Capability<seL4_EndpointObject, seL4_EndpointBits>& faultEndpoint,
            seL4_Word faultEndpointBadge,
            memory::vaddr_t entryPoint
        );
        void kill() noexcept;

        void handleFault(const seL4_MessageInfo_t& message) noexcept;

        pid_t getTid() const noexcept {return _tid;}
        std::shared_ptr<Process> getProcess() noexcept {return _process;}
        seL4_TCB getCap() const noexcept {return _tcbCap.get();}

        timer::Timestamp getStartTime() const noexcept {return _startTime;}

    private:
        explicit Thread(std::shared_ptr<Process> process);

        enum class Status {CREATED, STARTED, ZOMBIE};
        Status _status;

        pid_t _tid;
        std::shared_ptr<Process> _process;

        seL4_CPtr _faultEndpoint;
        Capability<seL4_TCBObject, seL4_TCBBits> _tcbCap;

        memory::ScopedMapping _stack;
        memory::ScopedMapping _ipcBuffer;

        timer::Timestamp _startTime;

        friend class Process;
};

}
