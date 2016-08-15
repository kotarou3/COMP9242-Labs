/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/io.h>
#include <sel4/types.h>

#define DMA_SIZE_BITS 22

/**
 * Initialises DMA memory
 * @param[in] paddr    The base physical address of the memory to use for DMA
 * @param[in] sizebits The size (1 << sizebits bytes) of the memory provided.
 * @return             0 on success
 */
int dma_init(seL4_Word paddr, int sizebits);

void *sos_dma_malloc(void* cookie, size_t size, int align, int cached, ps_mem_flags_t flags);
void sos_dma_free(void *cookie, void *addr, size_t size);
uintptr_t sos_dma_pin(void *cookie, void *addr, size_t size);
void sos_dma_unpin(void *cookie, void *addr, size_t size);
void sos_dma_cache_op(void *cookie, void *addr, size_t size, dma_cache_op_t op);
