#include <string>
#include <stdexcept>
#include <system_error>

#include <assert.h>
#include <errno.h>

extern "C" {
    #include <sos.h>
    #include <sel4/sel4.h>

    #include "internal/sys/debug.h"
}

#include "internal/async.h"
#include "internal/memory/layout.h"
#include "internal/process/Thread.h"
#include "internal/syscall/syscall.h"
#include "internal/process/Table.h"

namespace process {

////////////
// Thread //
////////////

Thread::Thread(std::shared_ptr<Process> process):
    _status(Status::CREATED),
    _process(process),
    _faultEndpoint(0),
    _stack(_process->maps.insert(
        0, memory::STACK_PAGES,
        memory::Attributes{.read = true, .write = true},
        memory::Mapping::Flags{.shared = false, .fixed = false, .stack = true}
    )),
    _ipcBuffer(_process->maps.insert(
        0, 1,
        memory::Attributes{
            .read = true,
            .write = true,
            .execute = false,
            .locked = true
        },
        memory::Mapping::Flags{.shared = false}
    ))
{
    kprintf(LOGLEVEL_DEBUG, "<Process %p>::<Thread %p> Created\n", process.get(), this);
}

Thread::~Thread() {
    // Free the fault endpoint if valid
    if (_status == Status::STARTED)
        assert(cspace_delete_cap(_process->_cspace.get(), _faultEndpoint) == CSPACE_NOERROR);

    // C++17: _process->_threads.erase(weak_from_this());
    for (auto thread = _process->_threads.begin(); thread != _process->_threads.end(); ++thread) {
        if (thread->expired()) {
            _process->_threads.erase(thread);
            break;
        }
    }

    if (_status == Status::CREATED)
        kprintf(LOGLEVEL_DEBUG, "<Process %p>::<Thread %p> Destroyed\n", _process.get(), this);
    else
        kprintf(LOGLEVEL_DEBUG, "<Process %p>::<Thread %p (%d)> Destroyed\n", _process.get(), this, _tid);
}

async::future<void> Thread::start(
    pid_t tid,
    const Capability<seL4_EndpointObject, seL4_EndpointBits>& faultEndpoint,
    seL4_Word faultEndpointBadge,
    memory::vaddr_t entryPoint
) {
    if (_status != Status::CREATED)
        throw std::logic_error("Thread already started once");

    return _process->pageDirectory.allocateAndMap(
        _ipcBuffer.getAddress(),
        memory::Attributes{
            .read = true,
            .write = true,
            .execute = false,
            .locked = true
        }
    ).then([=, &faultEndpoint](auto page) {
        const memory::MappedPage& ipcBufferMappedPage = page.get();

        // Copy the fault endpoint to the user app to allow IPC
        _faultEndpoint = cspace_mint_cap(
            _process->_cspace.get(), cur_cspace,
            faultEndpoint.get(), seL4_AllRights,
            seL4_CapData_Badge_new(faultEndpointBadge)
        );
        if (_faultEndpoint == CSPACE_NULL)
            throw std::system_error(ENOMEM, std::system_category(), "Failed to mint user fault endpoint");
        assert(_faultEndpoint == SOS_IPC_EP_CAP);

        // Configure the TCB
        int err = seL4_TCB_Configure(
            _tcbCap.get(), _faultEndpoint, 0,
            _process->_cspace->root_cnode, seL4_NilData,
            _process->pageDirectory.getCap(), seL4_NilData,
            ipcBufferMappedPage.getAddress(), ipcBufferMappedPage.getPage().getCap()
        );
        if (err != seL4_NoError) {
            assert(cspace_delete_cap(_process->_cspace.get(), _faultEndpoint) == CSPACE_NOERROR);
            throw std::system_error(ENOMEM, std::system_category(), "Failed to configure the TCB: " + std::to_string(err));
        }

        // Start the thread
        seL4_UserContext context = {
            .pc = entryPoint,
            .sp = _stack.getEnd()
        };
        err = seL4_TCB_WriteRegisters(_tcbCap.get(), true, 0, 2, &context);
        if (err != seL4_NoError) {
            assert(cspace_delete_cap(_process->_cspace.get(), _faultEndpoint) == CSPACE_NOERROR);
            throw std::system_error(ENOMEM, std::system_category(), "Failed to start the thread: " + std::to_string(err));
        }

        _status = Status::STARTED;
        _tid = tid;

        _startTime = timer::getTimestamp();

        kprintf(LOGLEVEL_DEBUG, "<Process %p>::<Thread %p (%d)> Started\n", _process.get(), this, _tid);
    });
}

void Thread::kill() noexcept {
    if (_status != Status::STARTED)
        return;

    _tcbCap.reset();
    assert(cspace_delete_cap(_process->_cspace.get(), _faultEndpoint) == CSPACE_NOERROR);

    _ipcBuffer.reset();
    _stack.reset();

    _status = Status::ZOMBIE;

    // TODO: Only supports single-threaded processes
    _process->_isZombie = true;
    std::shared_ptr<Process> parent = _process->_parent.lock();
    if (parent)
        parent->emitChildExit(_process);
    else
        ThreadTable::get().erase(_process->getPid());

    kprintf(LOGLEVEL_DEBUG, "<Process %p>::<Thread %p (%d)> Killed\n", _process.get(), this, _tid);
}

void Thread::handleFault(const seL4_MessageInfo_t& message) noexcept {
    switch (seL4_MessageInfo_get_label(message)) {
        case seL4_VMFault: {
            memory::vaddr_t pc = seL4_GetMR(0);
            memory::vaddr_t address = seL4_GetMR(1);

            // MR 3 contains data fault status register
            // http://infocenter.arm.com/help/topic/com.arm.doc.100511_0401_10_en/ric1447333676062.html

            memory::Attributes faultType = {0};
            if (seL4_GetMR(2))
                faultType.execute = true;
            if (seL4_GetMR(3) & (1 << 11))
                faultType.write = true;
            else
                faultType.read = true;

            uint8_t status = ((seL4_GetMR(3) & (1 << 12)) >> (12 - 5)) |
                ((seL4_GetMR(3) & (1 << 10)) >> (10 - 4)) |
                (seL4_GetMR(3) & 0b1111);
            if (status == 0b000001) {
                kprintf(LOGLEVEL_WARNING, "Would raise SIGBUS, but not implemented yet - killing thread\n");
                kill();
                break;
            } else if (status != 0b000101 && status != 0b000111) {
                kprintf(LOGLEVEL_ERR, "Unknown status flag: %02x\n", status);
            }

            try {
                auto result = _process->handlePageFault(address, faultType);

                if (_status == Status::ZOMBIE)
                    break;

                if (result.is_ready()) {
                    result.get();
                    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 0);
                    seL4_Reply(reply);
                } else {
                    seL4_CPtr replyCap = cspace_save_reply_cap(cur_cspace);
                    if (replyCap == CSPACE_NULL)
                        throw std::system_error(ENOMEM, std::system_category(), "Failed to save reply cap");

                    std::weak_ptr<Thread> thread = shared_from_this();
                    result.then([=](async::future<void> result) {
                        std::shared_ptr<Thread> _thread = thread.lock();
                        if (_thread && _thread->_status != Status::ZOMBIE) try {
                            result.get();
                            seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 0);
                            seL4_Send(replyCap, reply);
                        } catch (const std::exception& e) {
                            kprintf(LOGLEVEL_DEBUG, "Caught %s\n", e.what());

                            kprintf(LOGLEVEL_NOTICE,
                                "Segmentation fault at 0x%08x, pc = 0x%08x, %c%c%c\n",
                                address, pc,
                                faultType.read ? 'r' : '-',
                                faultType.write ? 'w' : '-',
                                faultType.execute ? 'x' : '-'
                            );

                            kprintf(LOGLEVEL_WARNING, "Would raise SIGSEGV, but not implemented yet - killing thread\n");
                            _thread->kill();
                        }

                        assert(cspace_free_slot(cur_cspace, replyCap) == CSPACE_NOERROR);
                    });
                }
            } catch (const std::exception& e) {
                kprintf(LOGLEVEL_DEBUG, "Caught %s\n", e.what());

                kprintf(LOGLEVEL_NOTICE,
                    "Segmentation fault at 0x%08x, pc = 0x%08x, %c%c%c\n",
                    address, pc,
                    faultType.read ? 'r' : '-',
                    faultType.write ? 'w' : '-',
                    faultType.execute ? 'x' : '-'
                );

                kprintf(LOGLEVEL_WARNING, "Would raise SIGSEGV, but not implemented yet - killing thread\n");
                kill();
            }

            break;
        }

        case seL4_NoFault: { // Syscall
            try {
                auto result = syscall::handle(
                    shared_from_this(),
                    seL4_GetMR(0),
                    seL4_MessageInfo_get_length(message) - 1,
                    &seL4_GetIPCBuffer()->msg[1]
                );

                if (_status == Status::ZOMBIE)
                    break;

                if (result.is_ready()) {
                    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
                    seL4_SetMR(0, result.get());
                    seL4_Reply(reply);
                } else {
                    seL4_CPtr replyCap = cspace_save_reply_cap(cur_cspace);
                    if (replyCap == CSPACE_NULL) {
                        seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
                        seL4_SetMR(0, -ENOMEM);
                        seL4_Reply(reply);
                        break;
                    }

                    std::weak_ptr<Thread> thread = shared_from_this();
                    result.then([replyCap, thread](async::future<int> result) {
                        std::shared_ptr<Thread> _thread = thread.lock();
                        if (_thread && _thread->_status != Status::ZOMBIE) {
                            seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
                            try {
                                seL4_SetMR(0, result.get());
                            } catch (...) {
                                seL4_SetMR(0, -syscall::exceptionToErrno(std::current_exception()));
                            }
                            seL4_Send(replyCap, reply);
                        }

                        assert(cspace_free_slot(cur_cspace, replyCap) == CSPACE_NOERROR);
                    });
                }
            } catch (...) {
                seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
                seL4_SetMR(0, -syscall::exceptionToErrno(std::current_exception()));
                seL4_Reply(reply);
            }
            break;
        }

        default:
            kprintf(LOGLEVEL_ERR, "Unknown fault type %u\n", seL4_MessageInfo_get_label(message));
            break;
    }
}

/////////////
// Process //
/////////////

Process::Process(std::shared_ptr<Process> parent):
    isSosProcess(false),
    _parent(parent),

    // Create a simple 1 level CSpace
    _cspace(cspace_create(1), [](cspace_t* cspace) {assert(cspace_destroy(cspace) == CSPACE_NOERROR);})
{
    assert(parent);

    if (!_cspace)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to create CSpace");

    // XXX: We shouldn't reserve pages, but currently our page table layout
    // requires this
    pageDirectory.reservePages(memory::MMAP_START, memory::MMAP_STACK_END);

    kprintf(LOGLEVEL_DEBUG, "<Process %p> Created\n", this);
}

Process::Process(bool isSosProcess):
    pageDirectory(seL4_CapInitThreadPD),
    isSosProcess(isSosProcess),
    _cspace(cur_cspace, [](cspace_t*) {})
{
    assert(isSosProcess);

    // Make sure all of SOS's page tables are allocated
    pageDirectory.reservePages(memory::MMAP_START, memory::MMAP_END);
}

Process::~Process() {
    // Clean up any zombie children
    for (std::weak_ptr<Process> child : _children) {
        std::shared_ptr<Process> _child = child.lock();
        assert(_child);

        if (_child->_isZombie)
            ThreadTable::get().erase(_child->getPid());
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

    return pageDirectory.makeResident(address, map.attributes).then([](auto page) {
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

std::shared_ptr<Process> getSosProcess() noexcept {
    static std::shared_ptr<Process> sosProcess(new Process(true));
    if (sosProcess->maps._process.expired()) {
        sosProcess->maps._process = sosProcess;

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
