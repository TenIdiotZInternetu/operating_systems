// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "../theap.h"
#include <np/utest.h>
#include <stdlib.h>

#define MIN_SIZE 8
#define START_SIZE (MIN_SIZE << 12)
#define MAX_ALLOCATIONS (1024 * 1024 * 1024 / START_SIZE) + (START_SIZE / MIN_SIZE)

/*
 * Tests that malloc() eventually exhausts memory.
 */
int main(void) {
    utest_start("heap/exhaust1");

    size_t allocation_count = 0;
    size_t allocation_size = START_SIZE;
    while (1) {
        // Stress proper alignment a little bit ;-)
        uint8_t* ptr = malloc(allocation_size - 1);
        if (ptr == NULL) {
            allocation_size = allocation_size / 2;
            if (allocation_size < MIN_SIZE) {
                break;
            }
            continue;
        }

        utest_check_malloc_result(ptr, allocation_size - 1);
        utest_check_malloc_writable(ptr);

        allocation_count++;

        if (allocation_count > MAX_ALLOCATIONS) {
            printf("Too many (%d) allocations succeeded.\n", allocation_count);
            utest_failed();
            // Unreachable
            return -1;
        }
    }

    utest_passed();

    return 0;
}
