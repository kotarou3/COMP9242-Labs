/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <exception>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <nfs/nfs.h>

extern "C" {
    #include <cspace/cspace.h>

    #include <cpio/cpio.h>
    #include <elf/elf.h>
    #include <sos.h>
    #include <sel4/sel4.h>

    #include "internal/dma.h"
    #include "internal/network.h"

    #include "internal/ut_manager/ut.h"

    #include <autoconf.h>

    #include "internal/sys/debug.h"
    #include "internal/sys/panic.h"
}

#include "internal/elf.h"
#include "internal/fs/ConsoleDevice.h"
#include "internal/fs/DebugDevice.h"
#include "internal/fs/DeviceFileSystem.h"
#include "internal/fs/FlatFileSystem.h"
#include "internal/fs/NFSFileSystem.h"
#include "internal/memory/FrameTable.h"
#include "internal/memory/PageDirectory.h"
#include "internal/process/Thread.h"
#include "internal/syscall/fs.h"
#include "internal/timer/timer.h"

/* To differencient between async and and sync IPC, we assign a
 * badge to the async endpoint. The badge that we receive will
 * be the bitwise 'OR' of the async endpoint badge and the badges
 * of all pending notifications. */
#define IRQ_EP_BADGE         (1 << (seL4_BadgeBits - 1))
/* All badged IRQs set high bet, then we use uniq bits to
 * distinguish interrupt sources */
#define IRQ_BADGE_NETWORK (1 << 0)
#define IRQ_BADGE_TIMER   (1 << 1)

#define TTY_NAME             CONFIG_SOS_STARTUP_APP
#define TTY_EP_BADGE         (101)

/* The linker will link this symbol to the start address  *
 * of an archive of attached applications.                */
extern char _cpio_archive[];

std::unique_ptr<process::Thread> _tty_start_thread;

seL4_CPtr _sos_ipc_ep_cap;
seL4_CPtr _sos_interrupt_ep_cap;

static void print_bootinfo(const seL4_BootInfo* info) {
    /* General info */
    kprintf(LOGLEVEL_INFO,"Info Page:  %p\n", info);
    kprintf(LOGLEVEL_INFO,"IPC Buffer: %p\n", info->ipcBuffer);
    kprintf(LOGLEVEL_INFO,"Node ID: %d (of %d)\n",info->nodeID, info->numNodes);
    kprintf(LOGLEVEL_INFO,"IOPT levels: %d\n",info->numIOPTLevels);
    kprintf(LOGLEVEL_INFO,"Init cnode size bits: %d\n", info->initThreadCNodeSizeBits);

    /* Cap details */
    kprintf(LOGLEVEL_INFO,"\nCap details:\n");
    kprintf(LOGLEVEL_INFO,"Type              Start      End\n");
    kprintf(LOGLEVEL_INFO,"Empty             0x%08x 0x%08x\n", info->empty.start, info->empty.end);
    kprintf(LOGLEVEL_INFO,"Shared frames     0x%08x 0x%08x\n", info->sharedFrames.start,
                                                   info->sharedFrames.end);
    kprintf(LOGLEVEL_INFO,"User image frames 0x%08x 0x%08x\n", info->userImageFrames.start,
                                                   info->userImageFrames.end);
    kprintf(LOGLEVEL_INFO,"User image PTs    0x%08x 0x%08x\n", info->userImagePTs.start,
                                                   info->userImagePTs.end);
    kprintf(LOGLEVEL_INFO,"Untypeds          0x%08x 0x%08x\n", info->untyped.start, info->untyped.end);

    /* Untyped details */
    kprintf(LOGLEVEL_INFO,"\nUntyped details:\n");
    kprintf(LOGLEVEL_INFO,"Untyped Slot       Paddr      Bits\n");
    for (size_t i = 0; i < info->untyped.end-info->untyped.start; i++) {
        kprintf(LOGLEVEL_INFO,"%3d     0x%08x 0x%08x %d\n", i, info->untyped.start + i,
                                                   info->untypedPaddrList[i],
                                                   info->untypedSizeBitsList[i]);
    }

    /* Device untyped details */
    kprintf(LOGLEVEL_INFO,"\nDevice untyped details:\n");
    kprintf(LOGLEVEL_INFO,"Untyped Slot       Paddr      Bits\n");
    for (size_t i = 0; i < info->deviceUntyped.end-info->deviceUntyped.start; i++) {
        kprintf(LOGLEVEL_INFO,"%3d     0x%08x 0x%08x %d\n", i, info->deviceUntyped.start + i,
                                                   info->untypedPaddrList[i + (info->untyped.end - info->untyped.start)],
                                                   info->untypedSizeBitsList[i + (info->untyped.end-info->untyped.start)]);
    }

    kprintf(LOGLEVEL_INFO,"-----------------------------------------\n\n");

    /* Print cpio data */
    kprintf(LOGLEVEL_INFO,"Parsing cpio data:\n");
    kprintf(LOGLEVEL_INFO,"--------------------------------------------------------\n");
    kprintf(LOGLEVEL_INFO,"| index |        name      |  address   | size (bytes) |\n");
    kprintf(LOGLEVEL_INFO,"|------------------------------------------------------|\n");
    for(int i = 0;; i++) {
        unsigned long size;
        const char *name;
        void *data;

        data = cpio_get_entry(_cpio_archive, i, &name, &size);
        if(data != NULL){
            kprintf(LOGLEVEL_INFO,"| %3d   | %16s | %p | %12d |\n", i, name, data, size);
        }else{
            break;
        }
    }
    kprintf(LOGLEVEL_INFO,"--------------------------------------------------------\n");
}

void start_first_process(const char* app_name, seL4_CPtr fault_ep) try {
    /* open the console device */
    fs::rootFileSystem->open("console", fs::FileSystem::OpenFlags{.read = true, .write = true})
        .then(fs::asyncExecutor, [=](auto file) {
            auto process = std::make_shared<process::Process>();
            auto openFile = std::make_shared<fs::OpenFile>(
                file.get(),
                fs::OpenFile::Flags{
                    .read = true,
                    .write = true
                }
            );

            /* attach the console device to stdin/out/err */
            assert(process->fdTable.insert(openFile) == STDIN_FILENO);
            assert(process->fdTable.insert(openFile) == STDOUT_FILENO);
            assert(process->fdTable.insert(openFile) == STDERR_FILENO);

            /* parse the cpio image */
            kprintf(LOGLEVEL_INFO, "\nStarting \"%s\"...\n", app_name);
            unsigned long elf_size;
            uint8_t* elf_base = (uint8_t*)cpio_get_file(_cpio_archive, app_name, &elf_size);
            conditional_panic(!elf_base, "Unable to locate cpio header");

            elf::load(*process, elf_base);

            _tty_start_thread = std::make_unique<process::Thread>(
                process, fault_ep, TTY_EP_BADGE, elf_getEntryPoint(elf_base)
            );
        });
} catch (const std::exception& e) {
    kprintf(LOGLEVEL_EMERG, "Caught: %s\n", e.what());
    panic("Caught exception while starting first process\n");
}

static void _sos_ipc_init(seL4_CPtr* ipc_ep, seL4_CPtr* async_ep){
    seL4_Word ep_addr, aep_addr;
    int err;

    /* Create an Async endpoint for interrupts */
    aep_addr = ut_alloc(seL4_EndpointBits);
    conditional_panic(!aep_addr, "No memory for async endpoint");
    err = cspace_ut_retype_addr(aep_addr,
                                seL4_AsyncEndpointObject,
                                seL4_EndpointBits,
                                cur_cspace,
                                async_ep);
    conditional_panic(err, "Failed to allocate c-slot for Interrupt endpoint");

    /* Bind the Async endpoint to our TCB */
    err = seL4_TCB_BindAEP(seL4_CapInitThreadTCB, *async_ep);
    conditional_panic(err, "Failed to bind ASync EP to TCB");


    /* Create an endpoint for user application IPC */
    ep_addr = ut_alloc(seL4_EndpointBits);
    conditional_panic(!ep_addr, "No memory for endpoint");
    err = cspace_ut_retype_addr(ep_addr,
                                seL4_EndpointObject,
                                seL4_EndpointBits,
                                cur_cspace,
                                ipc_ep);
    conditional_panic(err, "Failed to allocate c-slot for IPC endpoint");
}


static void _sos_init(seL4_CPtr* ipc_ep, seL4_CPtr* async_ep) try {
    seL4_Word dma_addr;
    seL4_Word low, high;
    int err;

    /* Retrieve boot info from seL4 */
    const seL4_BootInfo* _boot_info = seL4_GetBootInfo();
    conditional_panic(!_boot_info, "Failed to retrieve boot info\n");
    print_bootinfo(_boot_info);

    /* Initialise the untyped sub system and reserve memory for DMA */
    err = ut_table_init(_boot_info);
    conditional_panic(err, "Failed to initialise Untyped Table\n");
    /* DMA uses a large amount of memory that will never be freed */
    dma_addr = ut_steal_mem(DMA_SIZE_BITS);
    conditional_panic(dma_addr == 0, "Failed to reserve DMA memory\n");

    /* find available memory */
    ut_find_memory(&low, &high);

    /* Initialise the untyped memory allocator */
    ut_allocator_init(low, high);

    /* Initialise the cspace manager */
    err = cspace_root_task_bootstrap(ut_alloc, ut_free, ut_translate,
                                     malloc, free);
    conditional_panic(err, "Failed to initialise the c space\n");

    /* Initialise the frame table. Requires cspace to be initialised first */
    memory::FrameTable::init(low, high);

    /* Initialise DMA memory */
    err = dma_init(dma_addr, DMA_SIZE_BITS);
    conditional_panic(err, "Failed to intiialise DMA memory\n");

    /* Set stdin/out/err to the debug device */
    auto debugDevice = std::make_shared<fs::OpenFile>(
        std::make_shared<fs::DebugDevice>(),
        fs::OpenFile::Flags{
            .read = true,
            .write = true
        }
    );
    assert(process::getSosProcess().fdTable.insert(debugDevice) == STDIN_FILENO);
    assert(process::getSosProcess().fdTable.insert(debugDevice) == STDOUT_FILENO);
    assert(process::getSosProcess().fdTable.insert(debugDevice) == STDERR_FILENO);

    _sos_ipc_init(ipc_ep, async_ep);
} catch (const std::exception& e) {
    kprintf(LOGLEVEL_EMERG, "Caught: %s\n", e.what());
    panic("Caught exception during initialisation\n");
}

static inline seL4_CPtr badge_irq_ep(seL4_CPtr ep, seL4_Word badge) {
    seL4_CPtr badged_cap = cspace_mint_cap(cur_cspace, cur_cspace, ep, seL4_AllRights, seL4_CapData_Badge_new(badge | IRQ_EP_BADGE));
    conditional_panic(!badged_cap, "Failed to allocate badged cap");
    return badged_cap;
}

/*
 * Main entry point - called by crt.
 */
int main(void) {

    kprintf(LOGLEVEL_NOTICE, "\nSOS Starting...\n");

    _sos_init(&_sos_ipc_ep_cap, &_sos_interrupt_ep_cap);

    /* Initialise the network hardware */
    network_init(badge_irq_ep(_sos_interrupt_ep_cap, IRQ_BADGE_NETWORK));

    /* Initialise the timer */
    timer::init(badge_irq_ep(_sos_interrupt_ep_cap, IRQ_BADGE_TIMER));

    /* Initialise the device filesystem */
    auto deviceFileSystem = std::make_unique<fs::DeviceFileSystem>();
    fs::ConsoleDevice::mountOn(*deviceFileSystem, "console");

    /* Initialise the root filesystem */
    auto rootFileSystem = std::make_unique<fs::FlatFileSystem>();
    rootFileSystem->mount(std::move(deviceFileSystem));
    rootFileSystem->mount(std::make_unique<fs::NFSFileSystem>(CONFIG_SOS_GATEWAY, CONFIG_SOS_NFS_DIR));
    fs::rootFileSystem = std::move(rootFileSystem);

    /* Start the user application */
    start_first_process(TTY_NAME, _sos_ipc_ep_cap);

    /* Wait on synchronous endpoint for IPC */
    kprintf(LOGLEVEL_INFO, "\nSOS entering syscall loop\n");

    while (true) {
        seL4_Word badge;
        seL4_MessageInfo_t message = seL4_Wait(_sos_ipc_ep_cap, &badge);

        if (badge & IRQ_EP_BADGE) {
            // Interrupt
            if (badge & IRQ_BADGE_NETWORK)
                network_irq();
            if (badge & IRQ_BADGE_TIMER)
                timer::handleIrq();
        } else if (badge == TTY_EP_BADGE) {
            _tty_start_thread->handleFault(message);
        } else {
            kprintf(LOGLEVEL_ERR, "Rootserver got an unknown message\n");
        }
    }
}
