#include <string>
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

namespace process {

////////////
// Thread //
////////////

Thread::Thread(std::shared_ptr<Process> process, seL4_CPtr faultEndpoint, seL4_Word faultEndpointBadge, memory::vaddr_t entryPoint):
    _process(process),
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
    auto page = _process->pageDirectory.allocateAndMap(
        _ipcBuffer.getAddress(),
        memory::Attributes{
            .read = true,
            .write = true,
            .execute = false,
            .locked = true
        }
    );
    assert(page.is_ready()); // TODO: Fix this
    const memory::MappedPage& ipcBufferMappedPage = page.get();

    // Copy the fault endpoint to the user app to allow IPC
    _faultEndpoint = cspace_mint_cap(
        _process->_cspace.get(), cur_cspace,
        faultEndpoint, seL4_AllRights,
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
}

Thread::~Thread() {
    // If we haven't been moved away, free the fault endpoint
    if (_tcbCap)
        assert(cspace_delete_cap(_process->_cspace.get(), _faultEndpoint) == CSPACE_NOERROR);
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
                kprintf(LOGLEVEL_WARNING, "Would raise SIGBUS, but not implemented yet - halting thread\n");
                break;
            } else if (status != 0b000101 && status != 0b000111) {
                kprintf(LOGLEVEL_ERR, "Unknown status flag: %02x\n", status);
            }

            try {
                auto result = _process->handlePageFault(address, faultType);
                if (result.is_ready()) {
                    result.get();
                    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 0);
                    seL4_Reply(reply);
                } else {
                    seL4_CPtr replyCap = cspace_save_reply_cap(cur_cspace);
                    if (replyCap == CSPACE_NULL)
                        throw std::system_error(ENOMEM, std::system_category(), "Failed to save reply cap");

                    result.then([=](async::future<void> result) {
                        try {
                            result.get();

                            seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 0);
                            seL4_Send(replyCap, reply);
                            assert(cspace_free_slot(cur_cspace, replyCap) == CSPACE_NOERROR);
                        } catch (const std::exception& e) {
                            kprintf(LOGLEVEL_DEBUG, "Caught %s\n", e.what());

                            kprintf(LOGLEVEL_NOTICE,
                                "Segmentation fault at 0x%08x, pc = 0x%08x, %c%c%c\n",
                                address, pc,
                                faultType.read ? 'r' : '-',
                                faultType.write ? 'w' : '-',
                                faultType.execute ? 'x' : '-'
                            );

                            kprintf(LOGLEVEL_WARNING, "Would raise SIGSEGV, but not implemented yet - halting thread\n");

                            assert(cspace_free_slot(cur_cspace, replyCap) == CSPACE_NOERROR);
                        }
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

                kprintf(LOGLEVEL_WARNING, "Would raise SIGSEGV, but not implemented yet - halting thread\n");
            }

            break;
        }

        case seL4_NoFault: { // Syscall
            try {
                auto result = syscall::handle(
                    *this,
                    seL4_GetMR(0),
                    seL4_MessageInfo_get_length(message) - 1,
                    &seL4_GetIPCBuffer()->msg[1]
                );
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

                    result.then([replyCap](async::future<int> result) {
                        seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
                        try {
                            seL4_SetMR(0, result.get());
                        } catch (...) {
                            seL4_SetMR(0, -syscall::exceptionToErrno(std::current_exception()));
                        }
                        seL4_Send(replyCap, reply);

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

Process::Process():
    maps([this](memory::vaddr_t address) {this->pageDirectory.unmap(address);}),
    isSosProcess(false),

    // Create a simple 1 level CSpace
    _cspace(cspace_create(1), [](cspace_t* cspace) {assert(cspace_destroy(cspace) == CSPACE_NOERROR);})
{
    if (!_cspace)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to create CSpace");

    // XXX: We shouldn't reserve pages, but currently our page table layout
    // requires this
    pageDirectory.reservePages(memory::MMAP_START, memory::MMAP_STACK_END);
}

Process::Process(bool isSosProcess):
    pageDirectory(seL4_CapInitThreadPD),
    maps([this](memory::vaddr_t address) {this->pageDirectory.unmap(address);}),
    isSosProcess(isSosProcess),
    _cspace(cur_cspace, [](cspace_t*) {})
{
    assert(isSosProcess);

    // Reserve areas of memory already in-use
    // TODO: Do better
    memory::Mapping::Flags flags = {0};
    flags.fixed = true;
    flags.reserved = true;
    maps.insert(
        0,
        memory::numPages(memory::SOS_BRK_START + memory::SOS_INIT_AREA_SIZE),
        memory::Attributes{},
        flags
    ).release();

    // Make sure all of SOS's page tables are allocated
    pageDirectory.reservePages(memory::MMAP_START, memory::MMAP_END);
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

Process& getSosProcess() noexcept {
    static Process sosProcess(true);
    return sosProcess;
}

}
