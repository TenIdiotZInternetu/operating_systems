// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Charles University

/*
 * Tests that recycling ASIDs of unused address spaces works.
 */

#include <drivers/machine.h>
#include <exc.h>
#include <ktest.h>
#include <ktest/thread.h>
#include <mm/as.h>
#include <proc/thread.h>

/*
 * Such number that 2 * TEST_AS_COUNT is smaller than ASID size but
 * such that 3 * TEST_AS_COUNT is bigger than ASID size.
 * I.e. for 2 * TEST_AS_COUNT we do not need to steal ASIDs but after
 * stopping TEST_AS_COUNT address spaces and creating TEST_AS_COUNT new ones,
 * we should see recycling.
 *
 * We use HARDWARE_ASIDS_MAX for faster testing.
 */
#define TEST_AS_COUNT (3 * ((HARDWARE_ASIDS_MAX) >> 3))

// The trivial computation of TEST_AS_COUNT above does not work well for very
// small numbers.
_Static_assert(HARDWARE_ASIDS_MAX >= 16);

#define LOOPS 10

typedef struct {
    thread_t* thread;
    unative_t pattern;
    bool first_round;
    volatile bool terminate;
} worker_info_t;

static worker_info_t worker_info[TEST_AS_COUNT * 2];

static worker_info_t* get_my_worker_info(void) {
    thread_t* self = thread_get_current();
    for (size_t i = 0; i < TEST_AS_COUNT * 2; i++) {
        if (self == worker_info[i].thread) {
            return &worker_info[i];
        }
    }
    ktest_assert(false, "worker_info not found");

    return NULL;
}

// Barrier for starting all workers at once
static volatile size_t started_workers_count = 0;
static volatile bool start_workers_first = false;
static volatile bool start_workers_second = false;

static void announce_worker_started(void) {
    bool ipl = interrupts_disable();
    started_workers_count++;
    interrupts_restore(ipl);
}

static void announce_worker_terminated(void) {
    bool ipl = interrupts_disable();
    started_workers_count--;
    interrupts_restore(ipl);
}

static size_t get_started_worker_count(void) {
    bool ipl = interrupts_disable();
    size_t result = started_workers_count;
    interrupts_restore(ipl);
    return result;
}

static void* as_worker(void* ignored) {
    worker_info_t* info = get_my_worker_info();
    dprintk("Worker %pT (%u) started...\n", thread_get_current(), info->pattern);

    announce_worker_started();

    if (info->first_round) {
        while (!start_workers_first) {
            thread_yield();
        }
    } else {
        while (!start_workers_second) {
            thread_yield();
        }
    }

    volatile unative_t* data = (unative_t*)(PAGE_NULL_COUNT * PAGE_SIZE);

    unative_t i = 0;
    while (!info->terminate) {
        unative_t expected_pattern = (info->pattern << 16) | i;
        *data = expected_pattern;

        thread_yield();

        ktest_assert(*data == expected_pattern,
                "%pT: value mismatch (base_pattern=0x%x, i=0x%x, actual=0x%x, expected=0x%x)",
                info->thread, info->pattern, i, *data, expected_pattern);
    }

    announce_worker_terminated();

    return NULL;
}

void kernel_test(void) {
    ktest_start("as/recycle");

    printk("Will create %u different address spaces (ASID space size is %u) ...\n", 2 * TEST_AS_COUNT, HARDWARE_ASIDS_MAX);
    printk("Creating ");
    for (size_t i = 0; i < TEST_AS_COUNT * 2; i++) {
        worker_info[i].pattern = i;
        worker_info[i].first_round = true;
        ktest_thread_create_new_as_checked(&worker_info[i].thread, as_worker, NULL, 0, "worker", PAGE_SIZE * 4);
        printk(".");
    }
    printk(" done.\n");

    printk("Waiting for workers to actually start ...\n");

    while (get_started_worker_count() != 2 * TEST_AS_COUNT) {
        thread_yield();
    }

    start_workers_first = true;

    // Let the workers run
    for (size_t i = 0; i < LOOPS; i++) {
        thread_yield();
    }

    printk("Terminating half of these workers ...\n");

    // Terminate half of the workers
    for (size_t i = 0; i < TEST_AS_COUNT * 2; i += 2) {
        worker_info[i].terminate = true;
        ktest_thread_join_checked(worker_info[i].thread, NULL);
    }

    printk("Starting new workers ...\n");
    // Start new threads
    for (size_t i = 0; i < TEST_AS_COUNT * 2; i += 2) {
        worker_info[i].terminate = false;
        worker_info[i].first_round = false;
        ktest_thread_create_new_as_checked(&worker_info[i].thread, as_worker, NULL, 0, "worker", PAGE_SIZE * 4);
    }

    // And do it again
    printk("Waiting for second batch of workers to actually start ...\n");

    while (get_started_worker_count() != 2 * TEST_AS_COUNT) {
        thread_yield();
    }

    start_workers_second = true;

    // Let the workers run
    for (size_t i = 0; i < LOOPS; i++) {
        thread_yield();
    }

    // Terminate all of the workers
    for (size_t i = 0; i < TEST_AS_COUNT * 2; i++) {
        worker_info[i].terminate = true;
        ktest_thread_join_checked(worker_info[i].thread, NULL);
    }

    ktest_passed();
}
