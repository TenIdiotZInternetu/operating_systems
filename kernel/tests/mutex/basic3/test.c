// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Charles University

/*
 * Mutex test focusing on bad synchronization inside mutex implementation.
 *
 * We repeatedly reinitialize single mutex and lock/unlock it with a busy
 * wait before all mutex operations. This should eventually trigger the timer
 * interrupt _inside_ the mutex functions.
 *
 * With incorrect implementation we have a relatively high chance of switching
 * the mutex to inconsistent state.
 *
 * The worker thread does not do any looping as with some implementations
 * the inconsistent state could get "fixed" by repeated calls (seemingly
 * empty queue that would keep thread suspended forever could get retried
 * on next calls to mutex_unlock etc.).
 *
 * Instead the loop around creates a fresh thread and mutex pair for each
 * try, incrementing the busy wait duration.
 *
 * It is recommended to run this with different scheduler quantums. Increase
 * the amount of memory available to the OS if you do not recycle thread
 * stacks or have bump-pointer heap only.
 *
 * In a very unlikely case, the thread quantum could be exactly long enough to
 * reschedule the main thread always inside the section protected by the mutex,
 * hence never giving the other thread an opportunity to run. To prevent this,
 * the main thread -- once in a while -- yields outside of the critical section
 * allowing the worker to progress. However, we also check that the amount
 * of yields is kept low as the worker thread needs to access the mutex only
 * once and hence a single yield should be enough for any round-robin
 * implementation of thread scheduling.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/mutex.h>
#include <proc/thread.h>

/** Minimum duration of busy wait. */
#define WAIT_MIN 10

/** Maximum duration of busy wait (we create WAIT_MAX - WAIT_MIN threads!). */
#define WAIT_MAX 2000

/** Maximum loops until the main thread yields (see above). */
#define MAX_LOOPS_UNTIL_YIELD 500

/** Maximum yields until error is signalled (see above). */
#define MAX_YIELDS 10

/* Data are passed to the mutex via shared static variables. */
static mutex_t mutex;
static volatile size_t wait_duration = WAIT_MIN;

static void* worker(void* ignored) {
    ktest_thread_busy_wait(wait_duration);
    mutex_lock(&mutex);
    ktest_thread_busy_wait(wait_duration);
    mutex_unlock(&mutex);
    return NULL;
}

void kernel_test(void) {
    ktest_start("mutex/basic3");

    printk("Wait duration from %u to %u.\n", wait_duration, WAIT_MAX);
    while (wait_duration < WAIT_MAX) {
        errno_t err = mutex_init(&mutex);
        ktest_assert_errno(err, "mutex_init");

        thread_t* thread;
        ktest_thread_create_checked(&thread, worker, NULL, 0, "worker");

        // See explanation above.
        size_t loop_safety_counter = 0;
        size_t yield_counter = 0;
        while (!thread_has_finished(thread)) {
            mutex_lock(&mutex);
            mutex_unlock(&mutex);
            loop_safety_counter++;
            if (loop_safety_counter > MAX_LOOPS_UNTIL_YIELD) {
                loop_safety_counter = 0;
                yield_counter++;
                ktest_assert(yield_counter < MAX_YIELDS, "Too many yields required.");
                thread_yield();
            }
        }

        ktest_thread_join_checked(thread, NULL);
        mutex_destroy(&mutex);

        wait_duration++;
    }

    ktest_passed();
}
