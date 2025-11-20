// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Charles University

/*
 * Mutex test optimized for race conditions. Repeatedly locks and unlocks a
 * mutex from different threads. The mutex protects a shared variable that
 * should not be broken by repeated updates.
 *
 * This test focuses on a common implementation mistake and should hit
 * problematic code in 'mutex_lock' with a proper scheduler quantum.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/mutex.h>
#include <proc/thread.h>

#define LOOPS 200
#define WAIT_LOOPS_INSIDE 30
#define WAIT_LOOPS_OUTSIDE_MAX 419
#define THREAD_COUNT 20

static thread_t* threads[THREAD_COUNT];

static volatile size_t total_counter = 0;
static mutex_t total_counter_guard;

static void* worker(void* ignored) {
    for (int i = 0; i < LOOPS; i++) {
        // Try to deplete the quantum just before locking the mutex.
        ktest_thread_busy_wait_random(WAIT_LOOPS_OUTSIDE_MAX);

        // Update the counter with a delay between the load and store.
        mutex_lock(&total_counter_guard);
        size_t old_total_counter = total_counter;
        ktest_thread_busy_wait(WAIT_LOOPS_INSIDE);
        total_counter = old_total_counter + 1;
        mutex_unlock(&total_counter_guard);
    }

    return NULL;
}

void kernel_test(void) {
    ktest_start("mutex/race");

    errno_t mutex_err = mutex_init(&total_counter_guard);
    ktest_assert_errno(mutex_err, "mutex_init");

    size_t thread_count = 0;
    while (thread_count < THREAD_COUNT) {
        errno_t thread_err = thread_create(&threads[thread_count], worker, NULL, 0, "worker");
        if (thread_err == ENOMEM) {
            break;
        }

        ktest_assert_errno(thread_err, "thread_create");
        thread_count++;
    }

    for (size_t i = 0; i < thread_count; i++) {
        ktest_thread_join_checked(threads[i], NULL);
    }

    ktest_assert(total_counter == LOOPS * thread_count, "total_counter is broken");

    ktest_passed();
}
