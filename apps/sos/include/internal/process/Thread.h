#pragma once

#include <memory>
#include <unordered_map>

extern "C" {
    #include <cspace/cspace.h>
    #include <sel4/types.h>
}

#include "internal/async.h"
#include "internal/fs/FileDescriptor.h"
#include "internal/memory/Mappings.h"
#include "internal/memory/PageDirectory.h"
#include "internal/Capability.h"

namespace process {

class Process;
class Thread {
    public:
        Thread(std::shared_ptr<Process> process, seL4_CPtr faultEndpoint, seL4_Word faultEndpointBadge, memory::vaddr_t entryPoint);
        ~Thread();

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        Thread(Thread&& other) noexcept = default;
        Thread& operator=(Thread&& other) noexcept = default;

        void handleFault(const seL4_MessageInfo_t& message) noexcept;

        Process& getProcess() noexcept {return *_process;}
        seL4_TCB getCap() const noexcept {return _tcbCap.get();}

    private:
        std::shared_ptr<Process> _process;

        seL4_CPtr _faultEndpoint;
        Capability<seL4_TCBObject, seL4_TCBBits> _tcbCap;

        memory::ScopedMapping _stack;
        memory::ScopedMapping _ipcBuffer;
};

class Process {
    public:
        Process();

        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;

        async::future<void> handlePageFault(memory::vaddr_t, memory::Attributes attributes);
        async::future<void> pageFaultMultiple(memory::vaddr_t start, size_t pages, memory::Attributes attributes, std::shared_ptr<memory::ScopedMapping> map);

        memory::PageDirectory pageDirectory;
        memory::Mappings maps;
        fs::FDTable fdTable;

        const bool isSosProcess;

    private:
        explicit Process(bool isSosProcess);
        friend Process& getSosProcess() noexcept;

        std::unique_ptr<cspace_t, std::function<void (cspace_t*)>> _cspace;

        friend class Thread;
};

Process& getSosProcess() noexcept;

}
