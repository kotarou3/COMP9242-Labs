/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sel4/sel4.h>

#include "internal/sys/debug.h"

#include "internal/sys/execinfo.h" /*for backtrace()*/

static void sel4_abort(void) {
    kprintf(0, "seL4 root server aborted\n");

    /* Printout backtrace*/
    void *array[10] = {NULL};
    int size = 0;

    size = backtrace(array, 10);
    if (size) {

        kprintf(0, "Backtracing stack PCs:  ");

        for (int i = 0; i < size; i++) {
            kprintf(0, "0x%x  ", (unsigned int)array[i]);
        }
        kprintf(0, "\n");
    }

#if defined(CONFIG_DEBUG_BUILD)
    seL4_DebugHalt();
#endif
    while (1)
        ; /* We don't return after this */
}

long
sys_exit()
{
    abort();
    __builtin_unreachable();
}

long
sys_rt_sigprocmask()
{
    kprintf(0, "Ignoring call to %s\n", __FUNCTION__);
    return 0;
}

long
sys_gettid()
{
    kprintf(0, "Ignoring call to %s\n", __FUNCTION__);
    return 0;
}

long
sys_getpid()
{
    kprintf(0, "Ignoring call to %s\n", __FUNCTION__);
    return 0;
}

long
sys_set_tid_address()
{
    kprintf(0, "Ignoring call to %s\n", __FUNCTION__);
    return sys_gettid();
}

long
sys_tkill()
{
    kprintf(0, "%s assuming self kill\n", __FUNCTION__);
    sel4_abort();
    __builtin_unreachable();
}

long
sys_tgkill()
{
    kprintf(0, "%s assuming self kill\n", __FUNCTION__);
    sel4_abort();
    __builtin_unreachable();
}
