// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Tests that different AS are separated from each other.
 */

#include <drivers/machine.h>
#include <exc.h>
#include <ktest.h>
#include <ktest/barrier.h>
#include <ktest/thread.h>
#include <mm/as.h>
#include <mm/heap.h>
#include <proc/thread.h>

/*
 * Increase to HARDWARE_ASIDS_MAX + 5 if you recycle ASIDs of active address
 * spaces ;-)
 */
#ifndef TEST_AS_COUNT
#define TEST_AS_COUNT 20
#endif

#define LOOPS 100

typedef struct {
    thread_t* thread;
    unative_t pattern;
} worker_info_t;

static worker_info_t worker_info[TEST_AS_COUNT];

static worker_info_t* get_my_worker_info(void) {
    thread_t* self = thread_get_current();
    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        if (self == worker_info[i].thread) {
            return &worker_info[i];
        }
    }
    ktest_assert(false, "worker_info not found");

    return NULL;
}

// Barrier for starting all threads at once
static test_thread_barrier_t* barrier;

// Keeping track of progress of the actual workload
static test_progressbar_t progressbar;

static void* as_worker(void* ignored) {
    worker_info_t* info = get_my_worker_info();
    dprintk("Worker %pT (%u) started...\n", thread_get_current(), info->pattern);

    test_thread_barrier_wait(barrier);

    volatile unative_t* data = (unative_t*)(PAGE_NULL_COUNT * PAGE_SIZE);

    unative_t i = 0;
    do {
        unative_t expected_pattern = (info->pattern << 16) | i;
        *data = expected_pattern;

        thread_yield();

        unative_t data_now = *data;

        ktest_assert(data_now == expected_pattern,
                "%pT (%p): value mismatch (base_pattern=0x%x, i=0x%x, actual=0x%x, expected=0x%x)",
                info->thread, thread_get_as(info->thread), info->pattern, i, data_now, expected_pattern);

        i++;
        test_progressbar_step(&progressbar);
    } while (i < LOOPS);

    return NULL;
}

void kernel_test(void) {
    ktest_start("as/isolation");

    barrier = test_thread_barrier_create(TEST_AS_COUNT);
    ktest_assert(barrier != NULL, "out of memory when creating the barrier");

    test_progressbar_init(&progressbar, "Waiting for workers to finish", TEST_AS_COUNT * LOOPS);

    printk("Will create %u different address spaces ...\n", TEST_AS_COUNT);
    printk("  ...");
    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        worker_info[i].pattern = i;
        ktest_thread_create_new_as_checked(&worker_info[i].thread, as_worker, NULL, 0, "worker", PAGE_SIZE * 4);
        if ((i % 10) == 0) {
            printk(".");
        }
    }
    printk(" done.\n");

    printk("Waiting for workers to actually start ...\n");

    while (test_thread_barrier_get_waiting_for_count(barrier) > 0) {
        printk(".");
        thread_yield();
    }
    printk("\n");

    while (!test_progressbar_is_done(&progressbar)) {
        test_progressbar_print(&progressbar);
        thread_yield();
    }

    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        ktest_thread_join_checked(worker_info[i].thread, NULL);
    }

    ktest_passed();
}
