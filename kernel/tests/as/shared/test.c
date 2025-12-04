// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Tests that multiple threads can share an address space.
 *
 * Because we cannot expect that everyone would implement thread parameters,
 * we basically pass the parameters through a global array.
 *
 * To prevent the structure of thread_t being reused (i.e. some of the workers
 * finish before others are even created), we postpone the deallocation
 * (i.e., thread_join) until the end of the test.
 */

#include <debug/code.h>
#include <exc.h>
#include <ktest.h>
#include <ktest/thread.h>
#include <mm/as.h>
#include <proc/sem.h>
#include <proc/thread.h>

/*
 * We configure the tests in the following axes:
 *  - how many different AS we create
 *  - how many threads we launch per each AS
 *  - how big is the AS
 *  - how many times we check the mapping inside each thread
 *  - how many value we want to test inside AS (we use relatively
 *    high value of STEP to speed-up the test)
 */

#define TEST_AS_COUNT 5
#define TEST_AS_SIZE_PAGES 8
#define THREADS_PER_AS 5
#define STEP 1001
#define LOOPS 100

typedef struct parent_info parent_info_t;

/** Children threads. */
typedef struct {
    bool reverse;
    parent_info_t* parent;
    thread_t* thread;
    size_t counter;
} child_info_t;

/** Information about thread that created the new AS. */
struct parent_info {
    thread_t* thread;
    unative_t pattern;
    child_info_t children[THREADS_PER_AS];
};

static parent_info_t parents[TEST_AS_COUNT];

static parent_info_t* parent_get_my_info(void) {
    thread_t* self = thread_get_current();
    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        if (self == parents[i].thread) {
            return &parents[i];
        }
    }
    ktest_assert(false, "parent_info not found");

    return NULL;
}

static child_info_t* child_get_my_info(void) {
    thread_t* self = thread_get_current();
    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        for (size_t j = 0; j < THREADS_PER_AS; j++) {
            if (self == parents[i].children[j].thread) {
                return &parents[i].children[j];
            }
        }
    }
    ktest_assert(false, "children_info not found");

    return NULL;
}

/*
 * Accounting of runnint threads (to display progress).
 */

static size_t running_threads = 0;

static void running_threads_inc(void) {
    bool ipl = interrupts_disable();
    running_threads++;
    interrupts_restore(ipl);
}

static void running_threads_dec(void) {
    bool ipl = interrupts_disable();
    running_threads--;
    interrupts_restore(ipl);
}

static size_t running_threads_get_count(void) {
    bool ipl = interrupts_disable();
    size_t val = running_threads;
    interrupts_restore(ipl);
    return val;
}

/*
 * Ranges of virtual memory we check and value at specific position.
 */

static uintptr_t get_data_start_addr(void) {
    return PAGE_NULL_COUNT * PAGE_SIZE;
}

static uintptr_t get_data_end_addr(void) {
    return PAGE_NULL_COUNT * PAGE_SIZE + STEP * (PAGE_SIZE * TEST_AS_SIZE_PAGES / STEP);
}

static inline uint8_t get_value_at_addr(uintptr_t addr, unative_t pattern) {
    return (addr ^ pattern) & 0xff;
}

static void check_values(uint8_t* start, uint8_t* end, int step, unative_t pattern) {
    for (volatile uint8_t* it = start; it != end; it += step) {
        uint8_t expected = get_value_at_addr((uintptr_t)it, pattern);
        uint8_t actual = *it;
        ktest_assert(actual == expected,
                "%pT: value mismatch (at %p, pattern=%u (0x%x), actual=0x%x, expected=0x%x)",
                thread_get_current(), it, pattern, pattern, actual, expected);
        *it = expected;
    }
}

static bool all_children_has_finished(parent_info_t* parent) {
    for (size_t i = 0; i < THREADS_PER_AS; i++) {
        if (!thread_has_finished(parent->children[i].thread)) {
            return false;
        }
    }
    return true;
}

static void* worker_child(void* ignored) {
    running_threads_inc();
    child_info_t* info = child_get_my_info();

    dprintk("%pT [child %u/%u] running...\n", info->thread, info->parent->pattern, info->counter);

    uint8_t* data_start = (uint8_t*)get_data_start_addr();
    uint8_t* data_end = (uint8_t*)get_data_end_addr();

    for (size_t i = 0; i < LOOPS; i++) {
        ktest_assert(info != NULL, "");
        ktest_assert(info->parent != NULL, "");
        if (info->reverse) {
            check_values(data_end, data_start, -STEP, info->parent->pattern);
        } else {
            check_values(data_start, data_end, STEP, info->parent->pattern);
        }
        thread_yield();
    }

    running_threads_dec();
    return NULL;
}

static void* worker_parent(void* ignored) {
    running_threads_inc();
    parent_info_t* info = parent_get_my_info();

    dprintk("%pT [parent %u] running at %p...\n", info->thread, info->pattern, debug_get_stack_pointer());

    uint8_t* data_start = (uint8_t*)get_data_start_addr();
    uint8_t* data_end = (uint8_t*)get_data_end_addr();

    for (volatile uint8_t* it = data_start; it <= data_end; it += STEP) {
        *it = get_value_at_addr((uintptr_t)it, info->pattern);
    }

    dprintk("%pT [parent %u] spawning children...\n", info->thread, info->pattern);

    for (size_t i = 0; i < THREADS_PER_AS; i++) {
        info->children[i].counter = i;
        info->children[i].parent = info;
        info->children[i].reverse = (i % 2) == 0;
        ktest_thread_create_checked(&info->children[i].thread, worker_child, NULL, 0, "child");
    }

    for (size_t i = 0; i < LOOPS; i++) {
        check_values(data_start, data_end, STEP, info->pattern);
        thread_yield();
    }

    while (!all_children_has_finished(info)) {
        thread_yield();
    }

    running_threads_dec();
    return NULL;
}

void kernel_test(void) {
    ktest_start("as/shared");

    printk("Will create %u different address spaces ...\n", TEST_AS_COUNT);
    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        parents[i].pattern = i;
        ktest_thread_create_new_as_checked(&parents[i].thread, worker_parent, NULL, 0, "parent", TEST_AS_SIZE_PAGES * PAGE_SIZE);
    }

    thread_yield();

    while (true) {
        size_t running_count = running_threads_get_count();
        printk("There are still %u running worker threads (max at %u) ...\n",
                running_count, TEST_AS_COUNT * (THREADS_PER_AS + 1));
        if (running_count == 0) {
            break;
        }
        // Reduce when debugging - this keeps number of message relatively low
        for (size_t i = 0; i < 5; i++) {
            thread_yield();
        }
    }

    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        for (size_t j = 0; j < THREADS_PER_AS; j++) {
            ktest_thread_join_checked(parents[i].children[j].thread, NULL);
        }
        ktest_thread_join_checked(parents[i].thread, NULL);
    }

    ktest_passed();
}
