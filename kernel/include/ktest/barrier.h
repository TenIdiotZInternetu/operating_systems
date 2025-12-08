// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Charles University

#ifndef _KTEST_BARRIER_H
#define _KTEST_BARRIER_H

/*
 * Simple barrier and progress bar implementation
 *
 * It would be possible to use semaphore, however we cannot be sure that
 * its implementation would be complete for tests that need this barrier.
 */

#include <exc.h>
#include <mm/heap.h>
#include <proc/thread.h>

typedef struct {
    size_t size;
    size_t already_waiting;
    bool opened;
    // thread_t pointers added after this member (i.e. outside the struct)
    thread_t* threads[0];
} test_thread_barrier_t;

typedef struct {
    size_t total;
    size_t finished;
    const char* message;
} test_progressbar_t;

/** Create a barrier for given number of threads.
 *
 * Dynamically allocates memory, can fail (returns NULL then).
 */
static test_thread_barrier_t* test_thread_barrier_create(size_t size) {
    // The last one will pass through directly
    test_thread_barrier_t* barrier = kmalloc(sizeof(test_thread_barrier_t) + (size - 1) * sizeof(thread_t*));
    if (barrier == NULL) {
        return NULL;
    }
    barrier->size = size;
    barrier->already_waiting = 0;
    barrier->opened = false;
    return barrier;
}

/** Called when thread reaches the barrier. */
static inline void test_thread_barrier_wait(test_thread_barrier_t* barrier) {
    bool ipl = interrupts_disable();
    bool wake_up_everyone = false;
    assert(barrier->already_waiting < barrier->size);
    if (barrier->already_waiting + 1 == barrier->size) {
        // We are the last one arriving
        barrier->opened = true;
        wake_up_everyone = true;
    } else {
        barrier->threads[barrier->already_waiting] = thread_get_current();
        barrier->already_waiting++;
        thread_suspend();
    }
    interrupts_restore(ipl);

    if (wake_up_everyone) {
        for (size_t i = 0; i < barrier->already_waiting; i++) {
            (void)thread_wakeup(barrier->threads[i]);
        }
    }
}

/** Tell number of threads that have not yet reached the barrier. */
static inline size_t test_thread_barrier_get_waiting_for_count(test_thread_barrier_t* barrier) {
    bool ipl = interrupts_disable();
    size_t result = barrier->size - barrier->already_waiting;
    if (barrier->opened) {
        result = 0;
    }
    interrupts_restore(ipl);
    return result;
}

/** Initialize progress bar, string is not copied. */
static inline void test_progressbar_init(test_progressbar_t* progressbar, const char* msg, size_t jobs) {
    progressbar->message = msg;
    progressbar->total = jobs;
    progressbar->finished = 0;
}

static inline void test_progressbar_step(test_progressbar_t* progressbar) {
    bool ipl = interrupts_disable();
    (progressbar->finished)++;
    interrupts_restore(ipl);
}

static inline bool test_progressbar_is_done(test_progressbar_t* progressbar) {
    bool ipl = interrupts_disable();
    bool is_done = progressbar->finished == progressbar->total;
    interrupts_restore(ipl);

    return is_done;
}

static inline void test_progressbar_print(test_progressbar_t* progressbar) {
    bool ipl = interrupts_disable();
    size_t finished = progressbar->finished;
    interrupts_restore(ipl);

    printk("%s [%u done (about %d%%)] ...\n",
            progressbar->message, finished, 100 * finished / progressbar->total);
}

#endif
