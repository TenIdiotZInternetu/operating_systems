// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Basic virtual memory mapping test (even-sized address spaces only).
 *
 * Create threads with new AS and increasing size, check that it is
 * possible to access all memory and that accessing beyond that memory
 * kills the thread.
 */

#include <ktest.h>
#include <mm/as.h>
#include <proc/thread.h>

/*
 * Test address spaces with sizes in the range [MIN, MAX], but only require
 * the implementation to be able to create threads with address spaces with
 * sizes in the range [MIN, REQUIRED]. It is OK for the implementation to
 * fail with ENOMEM on larger address spaces.
 */
#ifndef AS_PAGES_MAX
#define AS_PAGES_MAX 50
#endif
#ifndef AS_PAGES_REQUIRED
#define AS_PAGES_REQUIRED 10
#endif
#ifndef AS_PAGES_MIN
#define AS_PAGES_MIN 2
#endif

/*
 * Do not touch every byte to speed-up the test a bit.
 */
#define STEP 251

static volatile bool accessible_memory_ok;

static inline uint8_t get_value_for_address(uintptr_t addr) {
    return ((addr / PAGE_SIZE) ^ (addr & 0xff)) & 0xff;
}

static void* as_worker(void* unused) {
    thread_t* self = thread_get_current();
    as_t* as = thread_get_as(self);
    size_t as_size = as_get_size(as);
    ktest_assert(as_size >= PAGE_SIZE, "AS size is too small: %u", as_size);

    // Skip the unmapped area at the beginning of the address space.
    uint8_t* as_start = (uint8_t*)(PAGE_NULL_COUNT * PAGE_SIZE);
    uint8_t* as_end = as_start + as_size;
    size_t as_size_pages = as_size / PAGE_SIZE;

    printk("as_worker(%u): writing to range [%p, %p)\n", as_size_pages, as_start, as_end);
    for (uint8_t* addr = as_start; addr < as_end; addr += STEP) {
        *addr = get_value_for_address((uintptr_t)addr);
    }

    printk("as_worker(%u): reading from range [%p, %p)\n", as_size_pages, as_start, as_end);
    for (uint8_t* addr = as_start; addr < as_end; addr += STEP) {
        uint8_t expected = get_value_for_address((uintptr_t)addr);
        uint8_t actual = *addr;
        ktest_assert(expected == actual, "mismatching value at %p", addr);
    }

    accessible_memory_ok = true;

    // Prevent instruction reordering across this boundary.
    __asm__ volatile("");

    // Try causing a fault by touching unmapped memory.
    *as_end = 0xff;

    printk("as_worker(%u): survived touching unmapped memory at %p!\n", as_size_pages, as_end);
    ktest_failed();

    return NULL;
}

void kernel_test(void) {
    ktest_start("as/basic");

    for (size_t as_pages = AS_PAGES_MIN; as_pages < AS_PAGES_MAX; as_pages += 2) {
        accessible_memory_ok = false;

        thread_t* worker;
        errno_t err = thread_create_new_as(&worker, as_worker, NULL, 0, "worker", as_pages * PAGE_SIZE);
        if ((err == ENOMEM) && (as_pages > AS_PAGES_REQUIRED)) {
            printk("Out of memory for AS with %u pages, skipping the rest.\n", as_pages);
            // We failed to create a thread with a larger address space, but
            // so far the test was passing for smaller address spaces.
            break;
        }

        ktest_assert_errno(err, "thread_create_new_as");

        err = thread_join(worker, NULL);
        ktest_assert(err == EKILLED, "as_worker(%u) should have been killed, but thread_join returned %s", as_pages, errno_as_str(err));

        ktest_assert(accessible_memory_ok, "as_worker(%u) killed when touching mapped memory", as_pages);
    }

    ktest_passed();
}
