#include <stdexcept>
#include <string>

#include <assert.h>
#include <errno.h>

#include "internal/memory/layout.h"
#include "internal/process/Thread.h"
#include "internal/syscall/syscall.h"

extern "C" {
    #include <sos.h>
    #include <sel4/sel4.h>

    #define verbose 5
    #include "internal/sys/debug.h"
    #include "internal/sys/panic.h"
}

namespace process {

////////////
// Thread //
////////////

Thread::Thread(std::shared_ptr<Process> process, seL4_CPtr faultEndpoint, seL4_Word faultEndpointBadge, memory::vaddr_t entryPoint):
    _process(process),
    _stack(_process->maps.insertScoped(
        0, memory::STACK_PAGES,
        memory::Attributes{.read = true, .write = true},
        memory::Mapping::Flags{.shared = false, .fixed = false, .stack = true}
    )),
    _ipcBuffer(_process->maps.insertScoped(
        0, 1,
        memory::Attributes{.read = true, .write = true},
        memory::Mapping::Flags{.shared = false}
    ))
{
    const memory::MappedPage& ipcBufferMappedPage = _process->pageDirectory.allocateAndMap(
        _ipcBuffer.getAddress(),
        memory::Attributes{.read = true, .write = true}
    );

    // Copy the fault endpoint to the user app to allow IPC
    _faultEndpoint = cspace_mint_cap(
        _process->_cspace.get(), cur_cspace,
        faultEndpoint, seL4_AllRights,
        seL4_CapData_Badge_new(faultEndpointBadge)
    );
    if (_faultEndpoint == CSPACE_NULL)
        throw std::runtime_error("Failed to mint user fault endpoint");
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
        throw std::runtime_error("Failed to configure the TCB: " + std::to_string(err));
    }

    // Start the thread
    seL4_UserContext context = {
        .pc = entryPoint,
        .sp = _stack.getEnd()
    };
    err = seL4_TCB_WriteRegisters(_tcbCap.get(), true, 0, 2, &context);
    if (err != seL4_NoError) {
        assert(cspace_delete_cap(_process->_cspace.get(), _faultEndpoint) == CSPACE_NOERROR);
        throw std::runtime_error("Failed to start the thread: " + std::to_string(err));
    }
}

Thread::~Thread() {
    // If we haven't been moved away, free the fault endpoint
    if (_tcbCap)
        assert(cspace_delete_cap(_process->_cspace.get(), _faultEndpoint) == CSPACE_NOERROR);
}

void Thread::handleFault(const seL4_MessageInfo_t& message) noexcept {
    seL4_CPtr replyCap = cspace_save_reply_cap(cur_cspace);

    switch (seL4_MessageInfo_get_label(message)) {
        case seL4_VMFault: {
            if (replyCap == CSPACE_NULL) {
                kprintf(0, "Would raise SIGSEGV, but not implemented yet - halting thread\n");
                break;
            }

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
                kprintf(0, "Would raise SIGBUS, but not implemented yet - halting thread\n");
                break;
            } else if (status != 0b000101 && status != 0b000111) {
                kprintf(0, "Unknown status flag: %02x\n", status);
            }

            try {
                _process->handlePageFault(address, faultType);
                seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 0);
                seL4_Send(replyCap, reply);
            } catch (const std::exception& e) {
                kprintf(1, "Caught %s\n", e.what());

                kprintf(
                    0, "Segmentation fault at 0x%08x, pc = 0x%08x, %c%c%c\n",
                    address, pc,
                    faultType.read ? 'r' : '-',
                    faultType.write ? 'w' : '-',
                    faultType.execute ? 'x' : '-'
                );

                kprintf(0, "Would raise SIGSEGV, but not implemented yet - halting thread\n");
            }

            break;
        }

        case seL4_NoFault: { // Syscall
            if (replyCap == CSPACE_NULL) {
                seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
                seL4_SetMR(0, -ENOMEM);
                seL4_Reply(reply);
                return;
            }

            int result = syscall::handle(
                *this,
                seL4_GetMR(0),
                seL4_MessageInfo_get_length(message) - 1,
                &seL4_GetIPCBuffer()->msg[1]
            );

            seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_SetMR(0, result);
            seL4_Send(replyCap, reply);
            break;
        }

        default:
            kprintf(0, "Unknown fault type %u\n", seL4_MessageInfo_get_label(message));
            break;
    }

    if (replyCap != CSPACE_NULL)
        cspace_free_slot(cur_cspace, replyCap);
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
        throw std::runtime_error("Failed to create CSpace");
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
    maps.insert(0, memory::MMAP_START / PAGE_SIZE, memory::Attributes{}, flags);
}

void Process::handlePageFault(memory::vaddr_t address, memory::Attributes cause) {
    address = memory::pageAlign(address);
    const memory::Mapping& map = maps.lookup(address);

    if (map.flags.reserved)
        throw std::runtime_error("Attempted to access a reserved address");
    if (map.flags.stack && address == map.start)
        throw std::runtime_error("Attempted to access the stack guard page");

    if (cause.execute && !map.attributes.execute)
        throw std::runtime_error("Attempted to execute a non-executable region");
    if (cause.read && !map.attributes.read)
        throw std::runtime_error("Attempted to read from a non-readable region");
    if (cause.write && !map.attributes.write)
        throw std::runtime_error("Attempted to write to a non-writeable region");

    try {
        pageDirectory.lookup(address);
    } catch (const std::out_of_range&) {
        // page isn't mapped
        pageDirectory.allocateAndMap(address, map.attributes);
    }
}

Process& getSosProcess() noexcept {
    static Process sosProcess(true);
    return sosProcess;
}

}
