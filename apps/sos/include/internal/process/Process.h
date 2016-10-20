#pragma once

#include <list>
#include <memory>
#include <set>

#include "internal/async.h"
#include "internal/fs/FileDescriptor.h"
#include "internal/memory/Mappings.h"
#include "internal/memory/PageDirectory.h"

namespace process {

class Thread;
class Process : public std::enable_shared_from_this<Process> {
    public:
        static std::shared_ptr<Process> create(std::shared_ptr<Process> parent) {
            auto result = std::shared_ptr<Process>(new Process(parent));
            parent->_children.insert(result);
            return result;
        }
        ~Process();

        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;

        Process(Process&&) = delete;
        Process& operator=(Process&&) = delete;

        std::shared_ptr<Thread> createThread();
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

        void _shrinkZombie() noexcept;

        bool _isZombie = false;

        std::weak_ptr<Process> _parent;
        std::set<std::weak_ptr<Process>, std::owner_less<std::weak_ptr<Process>>> _children;

        std::set<std::weak_ptr<Thread>, std::owner_less<std::weak_ptr<Thread>>> _threads;

        std::unique_ptr<cspace_t, std::function<void (cspace_t*)>> _cspace;

        std::list<ChildExitCallback> _childExitListeners;

        friend class Thread;
};

std::shared_ptr<Process> getSosProcess() noexcept;

}
