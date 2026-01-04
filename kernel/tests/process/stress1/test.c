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
#define ACTIONS_MAX 200
#define ACTIONS_MIN 100

static volatile bool all_started = false;
static size_t thread_count = 0;
static thread_t* threads[THREAD_COUNT];

static size_t process_count = 0;
static process_t* processes[PROCESS_COUNT];

static inline unative_t get_simple_rand(unative_t* seed) {
    *seed = (*seed * 1439) % 211 + 7;
    return (*seed) >> 4;
}

static inline unative_t get_simple_rand_range(unative_t* seed, unative_t lower, unative_t upper) {
    return lower + get_simple_rand(seed) % (upper - lower);
}

static inline void thread_wakeup_robust(thread_t* thread) {
    errno_t err = thread_wakeup(thread);
    if (err == EEXITED) {
        return;
    }
    ktest_assert_errno(err, "thread_wakeup");
}

static void* worker_with_random_action(void* ignored) {
    // Spin until all threads are created.
    while (!all_started) {
        thread_yield();
    }

    thread_t* myself = thread_get_current();
    unative_t seed = (((unative_t)myself) >> 4) & 0xffff;

    unsigned int loops = get_simple_rand_range(&seed, ACTIONS_MIN, ACTIONS_MAX);
    for (unsigned int i = 0; i < loops; i++) {
        printk("!");
        int action = get_simple_rand(&seed) % 7;
        switch (action) {
        case 0:
            thread_suspend();
            break;
        case 1:
            thread_wakeup_robust(threads[get_simple_rand(&seed) % thread_count]);
            break;
        default:
            thread_yield();
        }
    }

    return NULL;
}

static size_t get_running_count() {
    //
    // Without disabling interrupts, the number of running threads is only an
    // approximation (upper bound), but monotonically decreasing towards zero.
    //
    size_t result = 0;
    for (size_t i = 0; i < thread_count; i++) {
        if (!thread_has_finished(threads[i])) {
            result++;
        }
    }

    return result;
}

static void make_thread_name(const char* prefix, char* buffer, size_t size, size_t thread_index) {
    uint8_t digits[sizeof(size_t) * 8];

    // Convert worker index.
    size_t digit_count = 0;
    if (thread_index == 0) {
        digits[0] = 0;
        digit_count = 1;
    } else {
        while (thread_index > 0) {
            digits[digit_count] = thread_index % 10;
            thread_index /= 10;
            digit_count++;
        }
    }

    // Start with worker name.
    while ((size > 0) && (*prefix != 0)) {
        *buffer = *prefix;

        buffer++;
        prefix++;
        size--;
    }

    // Append worker index.
    while ((size > 0) && (digit_count > 0)) {
        *buffer = digits[digit_count - 1] + '0';

        buffer++;
        digit_count--;
        size--;
    }

    *buffer = 0;
}

void kernel_test(void) {
    ktest_start("process/stress1");

    while (true) {
        errno_t err1 = EINVAL;
        errno_t err2 = EINVAL;

        // Try to create a thread
        if (thread_count < THREAD_COUNT) {
            char name[THREAD_NAME_MAX_LENGTH + 1];
            make_thread_name("worker", name, THREAD_NAME_MAX_LENGTH + 1, thread_count);

            err1 = thread_create(&threads[thread_count], worker_with_random_action, NULL, 0, name);
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

    //
    // Signal the worker threads to stop spinning and start running actions.
    // Because some actions suspend their worker thread, we need to wake them
    // from the main test thread (the workers are independent).
    //
    all_started = true;

    size_t count = thread_count;
    while (count > 0) {
        // Try to wake up all "unfinished" threads.
        for (size_t i = 0; i < thread_count; i++) {
            if (!thread_has_finished(threads[i])) {
                thread_wakeup_robust(threads[i]);
            }
        }

        count = get_running_count();
        // printk(" ... %u kernel threads running ...\n", count);

        // Give proportionally more CPU time to workers.
        for (size_t i = 0; i < 3; i++) {
            thread_yield();
        }
    }

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
