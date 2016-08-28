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

    #define verbose 5
    #include "internal/sys/debug.h"
    #include "internal/sys/panic.h"
}

#include "internal/elf.h"
#include "internal/memory/FrameTable.h"
#include "internal/memory/PageDirectory.h"
#include "internal/process/Thread.h"
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
    kprintf(1, "Info Page:  %p\n", info);
    kprintf(1,"IPC Buffer: %p\n", info->ipcBuffer);
    kprintf(1,"Node ID: %d (of %d)\n",info->nodeID, info->numNodes);
    kprintf(1,"IOPT levels: %d\n",info->numIOPTLevels);
    kprintf(1,"Init cnode size bits: %d\n", info->initThreadCNodeSizeBits);

    /* Cap details */
    kprintf(1,"\nCap details:\n");
    kprintf(1,"Type              Start      End\n");
    kprintf(1,"Empty             0x%08x 0x%08x\n", info->empty.start, info->empty.end);
    kprintf(1,"Shared frames     0x%08x 0x%08x\n", info->sharedFrames.start,
                                                   info->sharedFrames.end);
    kprintf(1,"User image frames 0x%08x 0x%08x\n", info->userImageFrames.start,
                                                   info->userImageFrames.end);
    kprintf(1,"User image PTs    0x%08x 0x%08x\n", info->userImagePTs.start,
                                                   info->userImagePTs.end);
    kprintf(1,"Untypeds          0x%08x 0x%08x\n", info->untyped.start, info->untyped.end);

    /* Untyped details */
    kprintf(1,"\nUntyped details:\n");
    kprintf(1,"Untyped Slot       Paddr      Bits\n");
    for (size_t i = 0; i < info->untyped.end-info->untyped.start; i++) {
        kprintf(1,"%3d     0x%08x 0x%08x %d\n", i, info->untyped.start + i,
                                                   info->untypedPaddrList[i],
                                                   info->untypedSizeBitsList[i]);
    }

    /* Device untyped details */
    kprintf(1,"\nDevice untyped details:\n");
    kprintf(1,"Untyped Slot       Paddr      Bits\n");
    for (size_t i = 0; i < info->deviceUntyped.end-info->deviceUntyped.start; i++) {
        kprintf(1,"%3d     0x%08x 0x%08x %d\n", i, info->deviceUntyped.start + i,
                                                   info->untypedPaddrList[i + (info->untyped.end - info->untyped.start)],
                                                   info->untypedSizeBitsList[i + (info->untyped.end-info->untyped.start)]);
    }

    kprintf(1,"-----------------------------------------\n\n");

    /* Print cpio data */
    kprintf(1,"Parsing cpio data:\n");
    kprintf(1,"--------------------------------------------------------\n");
    kprintf(1,"| index |        name      |  address   | size (bytes) |\n");
    kprintf(1,"|------------------------------------------------------|\n");
    for(int i = 0;; i++) {
        unsigned long size;
        const char *name;
        void *data;

        data = cpio_get_entry(_cpio_archive, i, &name, &size);
        if(data != NULL){
            kprintf(1,"| %3d   | %16s | %p | %12d |\n", i, name, data, size);
        }else{
            break;
        }
    }
    kprintf(1,"--------------------------------------------------------\n");
}

void start_first_process(const char* app_name, seL4_CPtr fault_ep) try {
    auto process = std::make_shared<process::Process>();

    /* parse the cpio image */
    kprintf(1, "\nStarting \"%s\"...\n", app_name);
    unsigned long elf_size;
    uint8_t* elf_base = (uint8_t*)cpio_get_file(_cpio_archive, app_name, &elf_size);
    conditional_panic(!elf_base, "Unable to locate cpio header");

    elf::load(*process, elf_base);

    _tty_start_thread = std::make_unique<process::Thread>(
        process, fault_ep, TTY_EP_BADGE, elf_getEntryPoint(elf_base)
    );
} catch (const std::exception& e) {
    kprintf(0, "Caught: %s\n", e.what());
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
    if(verbose > 0){
        print_bootinfo(_boot_info);
    }

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

    /* TODO: Set stdout to the debug device */

    _sos_ipc_init(ipc_ep, async_ep);
} catch (const std::exception& e) {
    kprintf(0, "Caught: %s\n", e.what());
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

    kprintf(0, "\nSOS Starting...\n");

    _sos_init(&_sos_ipc_ep_cap, &_sos_interrupt_ep_cap);

    /* Initialise the network hardware */
    network_init(badge_irq_ep(_sos_interrupt_ep_cap, IRQ_BADGE_NETWORK));

    /* Initialise the timer */
    timer::init(badge_irq_ep(_sos_interrupt_ep_cap, IRQ_BADGE_TIMER));

    /* Start the user application */
    start_first_process(TTY_NAME, _sos_ipc_ep_cap);

    /* Wait on synchronous endpoint for IPC */
    kprintf(0, "\nSOS entering syscall loop\n");

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
            kprintf(0, "Rootserver got an unknown message\n");
        }
    }
}
