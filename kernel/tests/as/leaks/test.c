// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Basic AS leak test.
 */

#include <ktest.h>
#include <ktest/leak.h>
#include <ktest/thread.h>
#include <mm/as.h>
#include <proc/thread.h>

#define AS_SIZE (4 * PAGE_SIZE)

/*
 * Do not touch every byte to speed-up the test a bit.
 */
#define STEP 251

static inline uint8_t get_value_for_address(uintptr_t addr) {
    return ((addr / PAGE_SIZE) ^ (addr & 0xff)) & 0xff;
}

static void* as_worker(void* unused) {
    thread_t* self = thread_get_current();
    // Skip the unmapped area at the beginning of the address space.
    uint8_t* as_start = (uint8_t*)(PAGE_NULL_COUNT * PAGE_SIZE);
    uint8_t* as_end = as_start + AS_SIZE;

    printk("as_worker(%pT): writing to range [%p, %p)\n", self, as_start, as_end);
    for (uint8_t* addr = as_start; addr < as_end; addr += STEP) {
        *addr = get_value_for_address((uintptr_t)addr);
    }

    printk("as_worker(%pT): reading from range [%p, %p)\n", self, as_start, as_end);
    for (uint8_t* addr = as_start; addr < as_end; addr += STEP) {
        uint8_t expected = get_value_for_address((uintptr_t)addr);
        uint8_t actual = *addr;
        ktest_assert(expected == actual, "mismatching value at %p", addr);
    }

    return NULL;
}

static void leak_check(void* ignored) {
    thread_t* worker;
    errno_t rc = thread_create_new_as(&worker, as_worker, NULL, 0, "leak-worker", AS_SIZE);
    if (rc != EOK) {
        // This is actually okay, probably the leak test has inserted some
        // NULLs so that the thread was not created.
        return;
    }
    rc = thread_join(worker, NULL);
    if (rc != EOK) {
        // This might be okay, perhaps the leak test has inserted NULLs during
        // on demand allocation.
        ktest_assert(rc == EKILLED, "thread_join can return EOK or EKILLED only");
        return;
    }
    ktest_assert_errno(rc, "thread_join");
}

void kernel_test(void) {
    ktest_start("as/leaks");

    ktest_leak_check(leak_check, NULL);

    ktest_passed();
}
