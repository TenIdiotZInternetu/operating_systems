// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Charles University

/*
 * Tests that address spaces actually uses ASIDs of the CPU.
 */

#include <adt/bitmap.h>
#include <drivers/mmu.h>
#include <exc.h>
#include <ktest.h>
#include <ktest/thread.h>
#include <mm/as.h>
#include <mm/heap.h>
#include <proc/thread.h>

#define AS_SIZE (4 * PAGE_SIZE)
#define MAX_WORKERS 1024
#define MIN_WORKERS 10
#define MIN_DIFFERENT_ASIDS 8
#define LOOPS 10

typedef struct {
    thread_t* thread;
    size_t index;
    volatile bool initialized;
} worker_info_t;

static worker_info_t workers[MAX_WORKERS];
static volatile bool terminate_workers = false;

static bitmap_t seen_asids;
static bitmap_unit_t seen_asids_storage[BITMAP_GET_STORAGE_SIZE(HARDWARE_ASIDS_MAX)];

static void seen_asids_init(void) {
    bitmap_init(&seen_asids, HARDWARE_ASIDS_MAX, seen_asids_storage);
}

static void seen_asids_add(size_t index) {
    bool ipl = interrupts_disable();
    bitmap_set(&seen_asids, index, true);
    interrupts_restore(ipl);
}

static size_t seen_asids_get_count_unsafe(void) {
    size_t result = 0;
    for (size_t i = 0; i < bitmap_get_size(&seen_asids); i++) {
        if (bitmap_is_set(&seen_asids, i)) {
            result++;
        }
    }
    return result;
}

static void seen_asids_dump(void) {
    bool ipl = interrupts_disable();
    printk("Seen ASIDS: ");
    size_t count = 0;
    for (size_t i = 0; i < bitmap_get_size(&seen_asids); i++) {
        if (bitmap_is_set(&seen_asids, i)) {
            printk("X");
            count++;
        } else {
            printk(".");
        }
    }
    printk(" (count = %u).\n", count);
    interrupts_restore(ipl);
}

static worker_info_t* get_my_worker_info(void) {
    thread_t* self = thread_get_current();
    for (size_t i = 0; i < MAX_WORKERS; i++) {
        if (self == workers[i].thread) {
            return &workers[i];
        }
    }
    ktest_assert(false, "worker_info not found");

    return NULL;
}

static void* worker_main(void* ignored) {
    worker_info_t* info = get_my_worker_info();
    dprintk("Worker %pT started...\n", thread_get_current());

    // This ensures that even with on-demand page allocation we have
    // our memory ready.
    volatile size_t* temp = (volatile size_t*)(AS_SIZE - 32);
    *temp = info->index;
    ktest_assert(*temp == info->index, "unable to read back what was written");

    info->initialized = true;

    while (!terminate_workers) {
        seen_asids_add(mmu_get_current_asid());
        thread_yield();
    }

    return NULL;
}

void kernel_test(void) {
    ktest_start("as/asidusage");

    seen_asids_init();

    size_t worker_count = 0;
    for (; worker_count < MAX_WORKERS; worker_count++) {
        worker_info_t* worker = &workers[worker_count];
        worker->index = worker_count;
        worker->initialized = false;

        errno_t err = thread_create_new_as(&worker->thread, worker_main, NULL, 0, "worker", AS_SIZE);
        if (err != EOK) {
            break;
        }

        while (!worker->initialized) {
            thread_yield();
            if (thread_has_finished(worker->thread)) {
                break;
            }
        }
        if (thread_has_finished(worker->thread)) {
            // Already finished - we must assume the implementation
            // uses on-demand page mapping (and hope other issues are
            // caught by other tests). So we assume we have no more memory,
            // no need to create more threads.
            printk("Thread already finished, assuming on-demand page mapping.\n");
            (void)thread_join(worker->thread, NULL);
            break;
        }
    }

    ktest_assert(worker_count > MIN_WORKERS,
            "expecting to create at least %u workers but failed after %u",
            MIN_WORKERS, worker_count);

    thread_yield();
    terminate_workers = true;
    thread_yield();

    for (size_t i = 0; i < worker_count; i++) {
        ktest_thread_join_checked(workers[i].thread, NULL);
    }

    seen_asids_dump();

    size_t seen_asids_count = seen_asids_get_count_unsafe();
    ktest_assert(seen_asids_count > MIN_DIFFERENT_ASIDS,
            "we expect to see more than %u asids but only %u seen",
            MIN_DIFFERENT_ASIDS, seen_asids_count);

    ktest_passed();
}
