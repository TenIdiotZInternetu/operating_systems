// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Charles University

/*
 * Stress test for scheduler and thread management with emphasis on checking
 * that interrupt state is not modified.
 *
 * Create up to THREAD_COUNT threads that repeatedly suspend or wake-up
 * other threads. All threads should eventually finish.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/thread.h>

#define THREAD_COUNT 400
#define ACTIONS_MAX 400
#define ACTIONS_MIN 200

static volatile bool all_started = false;
static size_t thread_count;
static thread_t* threads[THREAD_COUNT];

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

static void action_suspend(unative_t* random_seed) {
    // Interrupts are enabled and should remain enabled when we wake up.
    thread_suspend();
    ktest_assert_interrupts_enabled();
}

static void action_suspend_no_interrupts(unative_t* random_seed) {
    bool saved_state = interrupts_disable();

    // Interrupts are disabled and should remain disabled when we wake up.
    thread_suspend();
    ktest_assert_interrupts_disabled();

    interrupts_restore(saved_state);
    ktest_assert_interrupts_enabled();
}

static void action_yield(unative_t* random_seed) {
    // Interrupts are enabled and should remain enabled when we get back.
    thread_yield();
    ktest_assert_interrupts_enabled();
}

static void action_yield_no_interrupts(unative_t* random_seed) {
    bool saved_state = interrupts_disable();

    // Interrupts are disabled and should remain disabled when we get back.
    thread_yield();
    ktest_assert_interrupts_disabled();

    interrupts_restore(saved_state);
    ktest_assert_interrupts_enabled();
}

static void action_wakeup(unative_t* random_seed) {
    thread_wakeup_robust(threads[get_simple_rand(random_seed) % thread_count]);
}

static void (*actions[])(unative_t*) = {
    &action_suspend,
    &action_suspend_no_interrupts,
    &action_yield,
    &action_yield_no_interrupts,
    &action_wakeup,
};

static int actions_count = sizeof(actions) / sizeof(actions[0]);

static void* worker_with_random_action(void* ignored) {
    //
    // Interrupts must be enabled when the worker starts running,
    // and must remain enabled after each call to thread API.
    //
    ktest_assert_interrupts_enabled();

    // Spin until all threads are created.
    while (!all_started) {
        thread_yield();
        ktest_assert_interrupts_enabled();
    }

    thread_t* myself = thread_get_current();
    ktest_assert_interrupts_enabled();

    // Perform a random number of random actions (with interrupts enabled).
    unative_t seed = (((unative_t)myself) >> 4) & 0xffff;
    unsigned int loops = get_simple_rand_range(&seed, ACTIONS_MIN, ACTIONS_MAX);
    for (unsigned int i = 0; i < loops; i++) {
        int action = get_simple_rand(&seed) % actions_count;
        actions[action](&seed);
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
        thread_t* thread = threads[i];
        bool thread_is_active = !thread_has_finished(thread);
        ktest_assert_interrupts_enabled();

        if (thread_is_active) {
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
    ktest_start("thread/interrupts");

    //
    // Interrupts must be enabled when the teststarts running,
    // and must remain enabled after each call to thread API.
    //
    ktest_assert_interrupts_enabled();

    for (thread_count = 0; thread_count < THREAD_COUNT; thread_count++) {
        char name[THREAD_NAME_MAX_LENGTH + 1];
        make_thread_name("worker", name, THREAD_NAME_MAX_LENGTH + 1, thread_count);

        errno_t err = thread_create(&threads[thread_count], worker_with_random_action, NULL, 0, name);
        ktest_assert_interrupts_enabled();

        if (err == ENOMEM) {
            break;
        }

        ktest_assert_errno(err, "thread_create");
        dprintk("Created thread #%u: %pT.\n", thread_count, threads[thread_count]);
    }

    printk("Created %u threads...\n", thread_count);

    //
    // Signal the worker threads to stop spinning and start running actions.
    // Because some actions suspend their worker thread, we need to wake them
    // from the main test thread (the workers are independent).
    //
    // We start with interrupts enabled and expect them to remain enabled
    // after each call to thread API.
    //
    all_started = true;

    size_t count = thread_count;
    while (count > 0) {
        // Try to wake up all "unfinished" threads.
        for (size_t i = 0; i < thread_count; i++) {
            thread_t* thread = threads[i];
            bool thread_is_active = !thread_has_finished(thread);
            ktest_assert_interrupts_enabled();

            if (thread_is_active) {
                thread_wakeup_robust(thread);
                ktest_assert_interrupts_enabled();
            }
        }

        count = get_running_count();
        printk(" ... %u threads running ...\n", count);

        // Give proportionally more CPU time to workers.
        for (size_t i = 0; i < 3; i++) {
            thread_yield();
            ktest_assert_interrupts_enabled();
        }
    }

    //
    // When joining threads, we start with interrupts enabled
    // and expect them to remain enabled after each join.
    //
    ktest_assert_interrupts_enabled();

    for (size_t i = 0; i < thread_count; i++) {
        ktest_thread_join_checked(threads[i], NULL);
        ktest_assert_interrupts_enabled();
    }

    ktest_passed();
}
