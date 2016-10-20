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
#include "internal/process/Process.h"
#include "internal/process/Table.h"
#include "internal/process/Thread.h"
#include "internal/syscall/syscall.h"

namespace process {

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
        _process->maps.lookup(_ipcBuffer.getAddress())
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

                if (_status == Status::ZOMBIE) {
                    if (_process->_isZombie)
                        _process->_shrinkZombie();
                    break;
                }

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
                        if (_thread) { // only reply if the thread's still alive
                            if (_thread->_status != Status::ZOMBIE) {
                                try {
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
                            } else if (_thread->_process->_isZombie) {
                                _thread->_process->_shrinkZombie();
                            }
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

                if (_status == Status::ZOMBIE) {
                    if (_process->_isZombie)
                        _process->_shrinkZombie();
                    break;
                }

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
                        if (_thread) {
                            if (_thread->_status != Status::ZOMBIE) {
                                seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
                                try {
                                    seL4_SetMR(0, result.get());
                                } catch (...) {
                                    seL4_SetMR(0, -syscall::exceptionToErrno(std::current_exception()));
                                }
                                seL4_Send(replyCap, reply);
                            } else if (_thread->_process->_isZombie) {
                                _thread->_process->_shrinkZombie();
                            }
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
            kprintf(LOGLEVEL_ERR, "Unknown fault type %u - killing thread\n", seL4_MessageInfo_get_label(message));
            kill();
            break;
    }
}

}
