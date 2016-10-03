#pragma once

#include <memory>
#include <set>

extern "C" {
    #include <cspace/cspace.h>
    #include <sel4/types.h>
}

#include "internal/async.h"
#include "internal/fs/FileDescriptor.h"
#include "internal/memory/Mappings.h"
#include "internal/memory/PageDirectory.h"
#include "internal/timer/timer.h"
#include "internal/Capability.h"

namespace process {

class Thread;
class Process : public std::enable_shared_from_this<Process> {
    public:
        static std::shared_ptr<Process> create(std::shared_ptr<Process> parent) {
            auto result = std::shared_ptr<Process>(new Process(parent));
            result->maps._process = result;
            parent->_children.insert(result);
            return result;
        }
        ~Process();

        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;

        Process(Process&&) = delete;
        Process& operator=(Process&&) = delete;

        const std::set<std::weak_ptr<Thread>, std::owner_less<std::weak_ptr<Thread>>> getThreads() const noexcept {return _threads;}

        using ChildExitCallback = std::function<bool (std::shared_ptr<Process>)>;
        void onChildExit(const ChildExitCallback& callback);
        void emitChildExit(std::shared_ptr<Process> process) noexcept;

        async::future<void> handlePageFault(memory::vaddr_t, memory::Attributes attributes);
        async::future<void> pageFaultMultiple(memory::vaddr_t start, size_t pages, memory::Attributes attributes, std::shared_ptr<memory::ScopedMapping> map);

        pid_t getPid() const noexcept;
        bool isZombie() const noexcept {return _isZombie;}

        memory::PageDirectory pageDirectory;
        memory::Mappings maps;
        fs::FDTable fdTable;

        std::string filename;

        const bool isSosProcess;

    private:
        explicit Process(std::shared_ptr<Process> parent);
        explicit Process(bool isSosProcess);
        friend std::shared_ptr<Process> getSosProcess() noexcept;

        bool _isZombie = false;

        std::weak_ptr<Process> _parent;
        std::set<std::weak_ptr<Process>, std::owner_less<std::weak_ptr<Process>>> _children;

        std::set<std::weak_ptr<Thread>, std::owner_less<std::weak_ptr<Thread>>> _threads;

        std::unique_ptr<cspace_t, std::function<void (cspace_t*)>> _cspace;

        std::list<ChildExitCallback> _childExitListeners;

        friend class Thread;
};

class Thread : public std::enable_shared_from_this<Thread> {
    public:
        template <typename ...Args>
        static std::shared_ptr<Thread> create(Args&& ...args) {
            auto result = std::shared_ptr<Thread>(new Thread(std::forward<Args>(args)...));
            result->_process->_threads.insert(result);
            // TODO: Only supports single-threaded processes
            assert(result->_process->_threads.size() == 1);
            return result;
        }
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
};

std::shared_ptr<Process> getSosProcess() noexcept;

}
