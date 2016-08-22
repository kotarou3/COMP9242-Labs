/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/****************************************************************************
 *
 *      $Id:  $
 *
 *      Description: Simple milestone 0 test.
 *
 *      Author:			Godfrey van der Linden
 *      Original Author:	Ben Leslie
 *
 ****************************************************************************/

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>
#include <sos.h>

#define NPAGES 1000
#define TEST_ADDRESS 0x20000000

static void pt_test(void) {
    /* check the stack is above phys mem */
    char buf1[PAGE_SIZE];
    assert((void*)buf1 > (void*)TEST_ADDRESS);

    /* heap test */
    size_t* buf2 = malloc(NPAGES * PAGE_SIZE * sizeof(size_t));
    assert(buf2);

    /* set */
    for (size_t i = 0; i < NPAGES; i++) {
	    buf2[i * PAGE_SIZE] = i;
    }

    /* check */
    for (size_t i = 0; i < NPAGES; i++) {
	    assert(buf2[i * PAGE_SIZE] == i);
    }

    free(buf2);
}

static int recursive(int n) {
    volatile int a[1000];
    int total = 0;
    for (int i = 0; i < 100; i++) a[i] = i;
    for (int i = 0; i < 100; i++) total += a[i];
    if (n > 0) recursive(n - 1);
    return total;
}

static void overflow(volatile char* a) {
    volatile char buf[PAGE_SIZE / 2];
    *a = 1;
    overflow(buf);
}

int main(void) {
    printf("task:\tHello world, I'm\ttty_test!\n");

    recursive(100);
    pt_test();
    printf("Test ok\n");

    volatile char a;
    overflow(&a);

    return 0;
}
