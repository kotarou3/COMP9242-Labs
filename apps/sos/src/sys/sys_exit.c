/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdlib.h>

#include "internal/sys/debug.h"

static void sel4_abort(void) {
    kprintf(LOGLEVEL_EMERG, "SOS aborting\n");

    kprintf(LOGLEVEL_EMERG, "Waiting for debugger by spinning...\n");
    while (1)
        ;
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
    kprintf(LOGLEVEL_WARNING, "Ignoring call to %s\n", __FUNCTION__);
    return 0;
}

long
sys_gettid()
{
    kprintf(LOGLEVEL_WARNING, "Ignoring call to %s\n", __FUNCTION__);
    return 0;
}

long
sys_getpid()
{
    kprintf(LOGLEVEL_WARNING, "Ignoring call to %s\n", __FUNCTION__);
    return 0;
}

long
sys_set_tid_address()
{
    kprintf(LOGLEVEL_WARNING, "Ignoring call to %s\n", __FUNCTION__);
    return sys_gettid();
}

long
sys_tkill()
{
    kprintf(LOGLEVEL_WARNING, "%s assuming self kill\n", __FUNCTION__);
    sel4_abort();
    __builtin_unreachable();
}

long
sys_tgkill()
{
    kprintf(LOGLEVEL_WARNING, "%s assuming self kill\n", __FUNCTION__);
    sel4_abort();
    __builtin_unreachable();
}
