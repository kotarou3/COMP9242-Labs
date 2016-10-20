#include <stdexcept>
#include <system_error>

#include <assert.h>
#include <errno.h>

extern "C" {
    #include <cspace/cspace.h>

    #include "internal/sys/debug.h"
}

#include "internal/async.h"
#include "internal/memory/layout.h"
#include "internal/process/Process.h"
#include "internal/process/Table.h"
#include "internal/process/Thread.h"

namespace process {

Process::Process(std::shared_ptr<Process> parent):
    pageDirectory(*this),
    maps(*this),
    isSosProcess(false),
    _parent(parent),

    // Create a simple 1 level CSpace
    _cspace(cspace_create(1), [](cspace_t* cspace) {assert(cspace_destroy(cspace) == CSPACE_NOERROR);})
{
    assert(parent);

    if (!_cspace)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to create CSpace");

    kprintf(LOGLEVEL_DEBUG, "<Process %p> Created\n", this);
}

// isSosProcess is only there so we can have 2 different constructors - one for normal processes, one for sos
Process::Process(bool isSosProcess):
    pageDirectory(*this, seL4_CapInitThreadPD),
    maps(*this),
    isSosProcess(isSosProcess),
    _cspace(cur_cspace, [](cspace_t*) {})
{
    assert(isSosProcess);

    // Make sure all of SOS's page tables are allocated
    pageDirectory.reservePages(memory::MMAP_START, memory::MMAP_END);
}

Process::~Process() {
    // Pass all children to init
    for (std::weak_ptr<Process> child : _children) {
        std::shared_ptr<Process> _child = child.lock();
        assert(_child);

        auto initProcess = ThreadTable::get().get(process::MIN_TID)->getProcess();
        initProcess->_children.insert(_child);
        _child->_parent = initProcess;
        if (_child->_isZombie)
            initProcess->emitChildExit(_child);
    }

    auto parent = _parent.lock();
    if (parent) {
        // C++17: parent->_children.erase(weak_from_this());
        for (auto child = parent->_children.begin(); child != parent->_children.end(); ++child) {
            if (child->expired()) {
                parent->_children.erase(child);
                break;
            }
        }
    }

    kprintf(LOGLEVEL_DEBUG, "<Process %p> Destroyed\n", this);
}

std::shared_ptr<Thread> Process::createThread() {
    std::shared_ptr<Thread> result(new Thread(shared_from_this()));

    _threads.insert(result);
    // TODO: Only supports single-threaded processes
    assert(_threads.size() == 1);

    return result;
}

void Process::onChildExit(const ChildExitCallback& callback) {
    // First check for any zombie children
    for (std::weak_ptr<Process> child : _children) {
        std::shared_ptr<Process> _child = child.lock();
        assert(_child);

        if (_child->_isZombie && callback(_child)) {
            ThreadTable::get().erase(_child->getPid());
            assert(_children.erase(child));
            return;
        }
    }

    _childExitListeners.push_back(callback);
}

void Process::emitChildExit(std::shared_ptr<Process> process) noexcept {
    assert(process->_isZombie);
    assert(_children.count(process) > 0);

    for (auto listener = _childExitListeners.begin(); listener != _childExitListeners.end(); ) {
        if ((*listener)(process)) {
            listener = _childExitListeners.erase(listener);

            ThreadTable::get().erase(process->getPid());
            _children.erase(process);
        } else {
            ++listener;
        }
    }

    kprintf(LOGLEVEL_WARNING, "Would send SIGCHLD to <Process %p>, but not implemented yet\n", this);
}

async::future<void> Process::handlePageFault(memory::vaddr_t address, memory::Attributes cause) {
    address = memory::pageAlign(address);
    const memory::Mapping& map = maps.lookup(address);

    if (map.flags.reserved)
        throw std::system_error(EFAULT, std::system_category(), "Attempted to access a reserved address");
    if (map.flags.stack && address == map.start)
        throw std::system_error(EFAULT, std::system_category(), "Attempted to access the stack guard page");

    if (cause.execute && !map.attributes.execute)
        throw std::system_error(EFAULT, std::system_category(), "Attempted to execute a non-executable region");
    if (cause.read && !map.attributes.read)
        throw std::system_error(EFAULT, std::system_category(), "Attempted to read from a non-readable region");
    if (cause.write && !map.attributes.write)
        throw std::system_error(EFAULT, std::system_category(), "Attempted to write to a non-writeable region");

    return pageDirectory.makeResident(address, map).then([](auto page) {
        (void)page.get();
    });
}

async::future<void> Process::pageFaultMultiple(memory::vaddr_t start, size_t pages, memory::Attributes attributes, std::shared_ptr<memory::ScopedMapping> map) {
    if (map)
        assert(map->getStart() <= start && start + pages * PAGE_SIZE <= map->getEnd());

    async::promise<void> promise;
    promise.set_value();
    auto future = promise.get_future();

    for (size_t p = 0; p < pages; ++p) {
        memory::vaddr_t addr = start + p * PAGE_SIZE;
        future = future.then([this, addr, attributes, map](async::future<void> result) {
            result.get();
            return this->handlePageFault(addr, attributes);
        });
    }

    if (isSosProcess && !future.is_ready()) {
        // Can't block SOS
        throw std::bad_alloc();
    }

    return future;
}

pid_t Process::getPid() const noexcept {
    if (isSosProcess)
        return 0;

    // TODO: Supports only single-threaded processes
    assert(_threads.size() > 0);
    auto thread = _threads.begin()->lock();

    assert(thread);
    return thread->getTid();
}

void Process::_shrinkZombie() noexcept {
    assert(_isZombie);

    fdTable.clear();
    maps.clear();
    pageDirectory.clear();
}

std::shared_ptr<Process> getSosProcess() noexcept {
    static std::shared_ptr<Process> sosProcess;
    if (!sosProcess) {
        sosProcess = std::shared_ptr<Process>(new Process(true));

        // Reserve areas of memory already in-use (not in constructor due to
        // memory::Mappings requiring a valid _process)
        // TODO: Do better
        memory::Mapping::Flags flags = {0};
        flags.fixed = true;
        flags.reserved = true;
        sosProcess->maps.insert(
            0,
            memory::numPages(memory::SOS_BRK_START + memory::SOS_INIT_AREA_SIZE),
            memory::Attributes{},
            flags
        ).release();
    }

    return sosProcess;
}

}
