// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Stress test for scheduler and thread management.
 *
 * Create up to THREAD_COUNT threads that repeatedly suspend or wake-up
 * other threads. All threads should eventually finish.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/process.h>
#include <proc/thread.h>

#define THREAD_COUNT 20
#define PROCESS_COUNT 4

// Hopefully much much longer than thread quantum
#define THREAD_LOOPS (THREAD_COUNT * 2000)

static size_t thread_count = 0;
static thread_t* threads[THREAD_COUNT];

static size_t process_count = 0;
static process_t* processes[PROCESS_COUNT];

static volatile int blackhole = 0;

static void* worker(void* ignored) {
    for (int i = 0; i < THREAD_LOOPS; i++) {
        if ((i % 100) == 0) {
            printk(".");
        }
        blackhole = i;
    }

    return NULL;
}

void kernel_test(void) {
    ktest_start("process/stress");

    while (true) {
        errno_t err1 = EINVAL;
        errno_t err2 = EINVAL;

        // Try to create a thread
        if (thread_count < THREAD_COUNT) {
            err1 = thread_create(&threads[thread_count], worker, NULL, 0, "worker");
            if (err1 != ENOMEM) {
                ktest_assert_errno(err1, "thread_create");
                dprintk("Created thread #%u: %pT.\n", thread_count, threads[thread_count]);
                thread_count++;
            }
        }

        // Try to create a process
        if (process_count < PROCESS_COUNT) {
            err2 = process_create(&processes[process_count], PROCESS_IMAGE_START, PROCESS_IMAGE_SIZE, PROCESS_MEMORY_SIZE);
            if (err2 != ENOMEM) {
                ktest_assert_errno(err2, "process_create");
                dprintk("Created process #%u: %pP.\n", process_count, processes[process_count]);
                process_count++;
            }
        }

        if ((err1 != EOK) && (err2 != EOK)) {
            break;
        }
    }

    printk("Created %u kernel threads and %u user processes ...\n", thread_count, process_count);

    for (size_t i = 0; i < process_count; i++) {
        int exit_status;
        errno_t err = process_join(processes[i], &exit_status);
        ktest_assert_errno(err, "process_join");
        ktest_assert(exit_status == 0, "unexpected non-zero exit status (got %d) for process #%u", exit_status, i);
    }

    for (size_t i = 0; i < thread_count; i++) {
        ktest_thread_join_checked(threads[i], NULL);
    }

    ktest_passed();
}
