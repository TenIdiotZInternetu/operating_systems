// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Trivial test that checks if the scheduler is fair. We run multiple
 * threads, each incrementing its own counter without yielding. We
 * expect all counters to contain similar values.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/thread.h>

static void* worker_impl(size_t index);

static void* worker_1(void* ignored) {
    return worker_impl(0);
}

static void* worker_2(void* ignored) {
    return worker_impl(1);
}

static void* worker_3(void* ignored) {
    return worker_impl(2);
}

static void* worker_4(void* ignored) {
    return worker_impl(3);
}

struct worker {
    const char* name;
    thread_t* thread;
    volatile size_t counter;
    volatile bool ready;
    void* (*func)(void*);
};

static struct worker workers[] = {
    { .name = "counter_1", .counter = 0, .ready = false, .func = &worker_1 },
    { .name = "counter_2", .counter = 0, .ready = false, .func = &worker_2 },
    { .name = "counter_3", .counter = 0, .ready = false, .func = &worker_3 },
    { .name = "counter_4", .counter = 0, .ready = false, .func = &worker_4 },
};

static volatile bool terminate = false;
static volatile bool start_work = false;

static void* worker_impl(size_t index) {
    workers[index].ready = true;

    while (!start_work) {
        thread_yield();
    }

    while (!terminate) {
        workers[index].counter++;
    }

    return NULL;
}

#define LOOPS (1 << 10)
#define LOOPS_PROGRESS (1 << 4)
#define WORKERS (sizeof(workers) / sizeof(workers[0]))

void kernel_test(void) {
    ktest_start("thread/fairness2");

    printk("Creating %u threads...\n", WORKERS);
    for (size_t index = 0; index < WORKERS; index++) {
        struct worker* worker = &workers[index];
        ktest_thread_create_checked(&worker->thread, worker->func, NULL, 0, worker->name);
    }

    printk("Waiting for threads to start...\n");
    bool all_ready = false;
    while (!all_ready) {
        thread_yield();

        all_ready = true;
        for (size_t index = 0; index < WORKERS; index++) {
            all_ready &= workers[index].ready;
        }
    }

    start_work = all_ready;

    printk("Doing actual work: ");
    for (size_t i = 0; i < LOOPS; i++) {
        if (i % LOOPS_PROGRESS == 0) {
            printk(".");
        }
    }

    printk(" done.\n");
    terminate = true;

    printk("Joining all threads...\n");
    for (size_t index = 0; index < WORKERS; index++) {
        ktest_thread_join_checked(workers[index].thread, NULL);
    }

    printk("Checking loop counts...\n");
    for (size_t index = 0; index < WORKERS; index++) {
        struct worker* worker = &workers[index];
        for (size_t index2 = 0; index2 < WORKERS; index2++) {
            struct worker* worker2 = &workers[index2];
            ktest_assert_in_range(worker->name, worker->counter, worker2->counter / 2, worker2->counter * 2);
        }
    }

    ktest_passed();
}
