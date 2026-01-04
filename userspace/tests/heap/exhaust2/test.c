// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "../theap.h"
#include <np/utest.h>
#include <stdlib.h>

#define MIN_SIZE 16
#define START_SIZE (MIN_SIZE << 12)
#define MAX_ALLOCATIONS (1024 * 1024 * 1024 / START_SIZE) + (START_SIZE / MIN_SIZE)
#define LOOPS 2

/*
 * Tests that malloc() eventually exhausts memory, frees it back to the
 * system and tries once again. Proper allocator must allow that such
 * operation can be tried several times.
 */

static size_t exhaust_and_free(void) {
    uintptr_t previous_block = 0;

    size_t allocation_size = START_SIZE;
    size_t allocation_count = 0;
    while (1) {
        // Stress proper alignment a little bit ;-)
        uintptr_t* ptr = malloc(allocation_size - 1);
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
            return 0;
        }

        ptr[0] = previous_block;
        previous_block = (uintptr_t)ptr;
    }

    // Free it back
    while (previous_block != 0) {
        uintptr_t* ptr = (uintptr_t*)previous_block;
        previous_block = ptr[0];
        free(ptr);
    }

    return allocation_count;
}

int main(void) {
    utest_start("heap/exhaust2");

    size_t last_count = exhaust_and_free();
    utest_assert(last_count > 0, "no allocation succeeded");
    printf("Allocated %d.\n", last_count);
    for (int i = 0; i < LOOPS; i++) {
        size_t count = exhaust_and_free();
        printf("Allocated %d.\n", count);
        utest_assert_fmt(count == last_count,
                "allocation counts differ (%u => %u)", last_count, count);
    }

    utest_passed();

    return 0;
}
