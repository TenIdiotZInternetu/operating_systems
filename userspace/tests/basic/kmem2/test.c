// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/types.h>
#include <np/utest.h>
#include <stddef.h>
#include <stdio.h>

/*
 * Checks that accessing kernel memory causes forced process termination.
 *
 * Before trying to access we run a rather long loop to ensure we are
 * rescheduled at least once and user mode of the CPU is not lost
 * during reschedule.
 */

static volatile uintptr_t kernel_memory_address = 0x80000020;
static volatile uint8_t blackhole;

// Hopefully longer than thread quantum
#define LOOPS 10000

int main(void) {
    utest_start("basic/kmem1");
    utest_expect_abort();

    for (int i = 0; i < LOOPS; i++) {
        if ((i % 100) == 0) {
            putchar('.');
        }
        blackhole = i & 0xFF;
    }
    puts(" done.\n");

    uint8_t* cause_page_fault = (uint8_t*)kernel_memory_address;
    blackhole = *cause_page_fault;

    puts("Survived access to kernel memory.\n");

    utest_failed();

    return 0;
}
