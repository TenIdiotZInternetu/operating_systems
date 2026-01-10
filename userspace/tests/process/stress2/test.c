// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>

/**
 * Stress test for processes and threads.
 *
 * Only spins for a reasonable amount of time to ensure we are at least
 * once forcefully rescheduled.
 *
 * We do not print the message about passing test as this is checked
 * by the kernel part.
 */

// Hopefully longer than thread quantum
#define LOOPS 10000

static volatile int blackhole = 0;

int main(void) {
    utest_start("process/stress2");

    for (int i = 0; i < LOOPS; i++) {
        if ((i % 100) == 0) {
            putchar('o');
        }
        blackhole = i;
    }

    return 0;
}
