/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdexcept>
#include <limits>

extern "C" {
    #include <autoconf.h>

    #include <cspace/cspace.h>
    #include <sel4/sel4.h>

    #include "internal/dma.h"
    #include "internal/network.h"
    #include "internal/ut_manager/ut.h"

    #include "internal/sys/debug.h"
    #include "internal/sys/panic.h"
}

#include "internal/globals.h"
#include "internal/fs/ConsoleDevice.h"
#include "internal/fs/DebugDevice.h"
#include "internal/fs/DeviceFileSystem.h"
#include "internal/fs/FlatFileSystem.h"
#include "internal/fs/NFSFileSystem.h"
#include "internal/memory/FrameTable.h"
#include "internal/memory/Swap.h"
#include "internal/process/Table.h"
#include "internal/process/Process.h"
#include "internal/syscall/process.h"
#include "internal/timer/timer.h"

const Capability<seL4_EndpointObject, seL4_EndpointBits>& getIpcEndpoint() noexcept {
    static Capability<seL4_EndpointObject, seL4_EndpointBits> _ipcEndpoint;
    return _ipcEndpoint;
}

namespace {
    constexpr seL4_Word IRQ_EP_BADGE =      (1 << (seL4_BadgeBits - 1));
    constexpr seL4_Word IRQ_BADGE_NETWORK = (1 << 0);
    constexpr seL4_Word IRQ_BADGE_TIMER =   (1 << 1);

    // NFSv2 limits us to 2 GiB - 1 B
    constexpr size_t SWAP_SIZE = memory::pageAlign(2U * 1024 * 1024 * 1024 - 1);

    const Capability<seL4_AsyncEndpointObject, seL4_EndpointBits>& _getIrqEndpoint() noexcept {
        static Capability<seL4_AsyncEndpointObject, seL4_EndpointBits> _irqEndpoint;
        return _irqEndpoint;
    }

    void _printBootInfo(const seL4_BootInfo* info) noexcept {
        // General info
        kprintf(LOGLEVEL_INFO, "Info Page:  %p\n", info);
        kprintf(LOGLEVEL_INFO, "IPC Buffer: %p\n", info->ipcBuffer);
        kprintf(LOGLEVEL_INFO, "Node ID: %d (of %d)\n", info->nodeID, info->numNodes);
        kprintf(LOGLEVEL_INFO, "IOPT levels: %d\n", info->numIOPTLevels);
        kprintf(LOGLEVEL_INFO, "Init cnode size bits: %d\n", info->initThreadCNodeSizeBits);

        // Cap details
        kprintf(LOGLEVEL_INFO, "\nCap details:\n");
        kprintf(LOGLEVEL_INFO, "Type              Start      End\n");
        kprintf(LOGLEVEL_INFO, "Empty             0x%08x 0x%08x\n", info->empty.start, info->empty.end);
        kprintf(LOGLEVEL_INFO, "Shared frames     0x%08x 0x%08x\n", info->sharedFrames.start, info->sharedFrames.end);
        kprintf(LOGLEVEL_INFO, "User image frames 0x%08x 0x%08x\n", info->userImageFrames.start, info->userImageFrames.end);
        kprintf(LOGLEVEL_INFO, "User image PTs    0x%08x 0x%08x\n", info->userImagePTs.start, info->userImagePTs.end);
        kprintf(LOGLEVEL_INFO, "Untypeds          0x%08x 0x%08x\n", info->untyped.start, info->untyped.end);

        // Untyped details
        kprintf(LOGLEVEL_INFO, "\nUntyped details:\n");
        kprintf(LOGLEVEL_INFO, "Untyped Slot       Paddr      Bits\n");
        for (size_t i = 0; i < info->untyped.end-info->untyped.start; i++) {
            kprintf(LOGLEVEL_INFO,
                "%3zu     0x%08x 0x%08x %d\n",
                i,
                info->untyped.start + i,
                info->untypedPaddrList[i],
                info->untypedSizeBitsList[i]
            );
        }

        // Device untyped details
        kprintf(LOGLEVEL_INFO, "\nDevice untyped details:\n");
        kprintf(LOGLEVEL_INFO, "Untyped Slot       Paddr      Bits\n");
        for (size_t i = 0; i < info->deviceUntyped.end-info->deviceUntyped.start; i++) {
            kprintf(LOGLEVEL_INFO,
                "%3zu     0x%08x 0x%08x %d\n",
                i,
                info->deviceUntyped.start + i,
                info->untypedPaddrList[i + (info->untyped.end - info->untyped.start)],
                info->untypedSizeBitsList[i + (info->untyped.end-info->untyped.start)]
            );
        }

        kprintf(LOGLEVEL_INFO,"-----------------------------------------\n");
    }

    void _init() noexcept {
        // Retrieve boot info from seL4
        const seL4_BootInfo* bootInfo = seL4_GetBootInfo();
        conditional_panic(!bootInfo, "Failed to retrieve boot info\n");
        _printBootInfo(bootInfo);

        // Initialise the untyped sub system
        conditional_panic(
            ut_table_init(bootInfo),
            "Failed to initialise Untyped Table\n"
        );

        // DMA uses a large amount of memory that will never be freed
        seL4_Word dmaAddr = ut_steal_mem(DMA_SIZE_BITS);
        conditional_panic(!dmaAddr, "Failed to reserve DMA memory\n");

        // Find the available memory
        seL4_Word low, high;
        ut_find_memory(&low, &high);

        // Initialise the untyped memory allocator
        ut_allocator_init(low, high);

        // Initialise the cspace manager
        conditional_panic(
            cspace_root_task_bootstrap(ut_alloc, ut_free, ut_translate, malloc, free),
            "Failed to initialise cspace\n"
        );

        // Initialise the frame table
        memory::FrameTable::init(low, high);

        // Initialise DMA memory
        conditional_panic(
            dma_init(dmaAddr, DMA_SIZE_BITS),
            "Failed to initialise DMA memory\n"
        );

        // Bind the IRQ endpoint to our TCB
        conditional_panic(
            seL4_TCB_BindAEP(seL4_CapInitThreadTCB, _getIrqEndpoint().get()),
            "Failed to bind IRQ EP to TCB"
        );

        // Set stdin/out/err to the debug device
        auto debugDevice = std::make_shared<fs::OpenFile>(
            std::make_shared<fs::DebugDevice>(),
            fs::OpenFile::Flags{
                .read = true,
                .write = true
            }
        );
        assert(process::getSosProcess()->fdTable.insert(debugDevice) == STDIN_FILENO);
        assert(process::getSosProcess()->fdTable.insert(debugDevice) == STDOUT_FILENO);
        assert(process::getSosProcess()->fdTable.insert(debugDevice) == STDERR_FILENO);
    }

    inline Capability<seL4_AsyncEndpointObject, seL4_EndpointBits> _getBadgedIrqEndpoint(seL4_Word badge) noexcept {
        seL4_CPtr badgedCap = cspace_mint_cap(
            cur_cspace, cur_cspace,
            _getIrqEndpoint().get(), seL4_AllRights,
            seL4_CapData_Badge_new(badge | IRQ_EP_BADGE)
        );
        conditional_panic(!badgedCap, "Failed to allocate badged cap");
        return Capability<seL4_AsyncEndpointObject, seL4_EndpointBits>(0, badgedCap);
    }
}

int main() noexcept {
    kprintf(LOGLEVEL_NOTICE, "\nSOS Starting...\n");
    _init();

    // Initialise the network hardware
    network_init(_getBadgedIrqEndpoint(IRQ_BADGE_NETWORK).release().second);

    // Initialise the timer
    timer::init(_getBadgedIrqEndpoint(IRQ_BADGE_TIMER));

    // Initialise the device filesystem
    auto deviceFileSystem = std::make_unique<fs::DeviceFileSystem>();
    fs::ConsoleDevice::mountOn(*deviceFileSystem, "console");

    // Initialise the root filesystem with the device filesystem and NFS
    auto rootFileSystem = std::make_unique<fs::FlatFileSystem>();
    rootFileSystem->mount(std::move(deviceFileSystem));
    rootFileSystem->mount(std::make_unique<fs::NFSFileSystem>(CONFIG_SOS_GATEWAY, CONFIG_SOS_NFS_DIR));
    fs::rootFileSystem = std::move(rootFileSystem);

    // Initialise the swap file
    fs::rootFileSystem->open(
        "pagefile",
        fs::FileSystem::OpenFlags{
            .read = true,
            .write = true,
            .direct = true,
            .createOnMissing = true,
            .mode = S_IRUSR | S_IWUSR
        }
    ).then([](auto file) noexcept {
        memory::Swap::get().addBackingStore(file.get(), SWAP_SIZE);
    });

    // Start init
    process::getSosProcess()->onChildExit([](auto process) [[noreturn]] noexcept -> bool {
        assert(process->getPid() == process::MIN_TID);
        panic("init exited");
        __builtin_unreachable();
    });
    const char* initPath = CONFIG_SOS_STARTUP_APP;
    syscall::process_create(process::getSosProcess(), reinterpret_cast<memory::vaddr_t>(initPath))
        .then([](auto pid) noexcept {
            assert(pid.get() == process::MIN_TID);
        });

    // Wait on synchronous endpoint for IPC
    kprintf(LOGLEVEL_INFO, "\nSOS entering syscall loop\n");

    while (true) {
        seL4_Word badge;
        seL4_MessageInfo_t message = seL4_Wait(getIpcEndpoint().get(), &badge);

        if (badge & IRQ_EP_BADGE) {
            // Interrupt
            if (badge & IRQ_BADGE_NETWORK)
                network_irq();
            if (badge & IRQ_BADGE_TIMER)
                timer::handleIrq();
        } else {
            process::ThreadTable::get().get(badge)->handleFault(message);
        }
    }
}
