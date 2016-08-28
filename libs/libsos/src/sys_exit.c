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
#include "sos.h"

static void
sel4_abort()
{
    printf("sos aborting\n");
    seL4_DebugHalt();
    while(1); /* We don't return after this */
}

long
sys_rt_sigprocmask() {
    /* abort messages with signals in order to kill itself */
    __builtin_unreachable();
}

long
sys_gettid() {
    /* return dummy for now */
    return 0;
}

long
sys_getpid() {
    /* assuming process IDs are the same as thread IDs*/
    return sys_gettid();
}

long
sys_set_tid_address() {
    return sys_gettid();
}

long
sys_exit()
{
    abort();
    __builtin_unreachable();
}

long
sys_exit_group()
{
    abort();
    __builtin_unreachable();
}

long
sys_tkill()
{
    sel4_abort();
    __builtin_unreachable();
}

long
sys_tgkill()
{
    sel4_abort();
    __builtin_unreachable();
}
