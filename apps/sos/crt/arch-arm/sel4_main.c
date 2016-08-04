/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <elf.h>
#include <elf/elf32.h>
#include <limits.h>
#include <sel4/sel4.h>
#include <stdlib.h>

int main(void);
void exit(int code);

void _init() __attribute__((weak));
void _fini() __attribute__((weak));
_Noreturn int __libc_start_main(int (*)(), int, char **,
    void (*)(), void(*)(), void(*)());

void __attribute__((externally_visible)) _sel4_main(seL4_BootInfo *bi) {
    seL4_InitBootInfo(bi);

    #if UINTPTR_MAX != 0xffffffff
        #error Only 32-bit supported
    #endif
    struct Elf32_Header *elfHeader = (struct Elf32_Header *)0x10000;
    size_t argv[] = {
        // argv
        0,
        // envp
        0,
        // auxv (Needed for exception handling)
        AT_PHDR, (size_t)elf32_getProgramHeaderTable(elfHeader),
        AT_PHENT, sizeof(Elf32_Phdr),
        AT_PHNUM, elf32_getNumProgramHeaders(elfHeader),
        AT_NULL, 0
    };

    __libc_start_main(main, 0, (void *)argv, _init, _fini, NULL);
    /* should not get here */
    while(1);
}
