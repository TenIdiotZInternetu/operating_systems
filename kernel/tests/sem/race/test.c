// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Test that semaphores work using the producer/consumer synchronization
 * problem. The test spawns different numbers of producers and consumers
 * in several stages, processing a fixed number of items. At the end of
 * the test, all threads should terminate and the shared queue should be
 * empty, because the total number of producers equals the total number
 * of consumers.
 *
 * Note that we don't implement the queue itself, we just capture its
 * state using semaphores.
 *
 * This test focuses on a common implementation mistake and should hit
 * problematic code in 'sem_wait' with a proper scheduler quantum.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/sem.h>
#include <proc/thread.h>

#define WORKER_ITEM_COUNT 100
#define BASE_THREAD_COUNT 15
#define QUEUE_SIZE (BASE_THREAD_COUNT * 2)
#define MAX_THREAD_COUNT (6 * BASE_THREAD_COUNT)

#define WAIT_LOOPS BASE_THREAD_COUNT
#define WAIT_LOOPS_OUTSIDE_MAX 419

static sem_t queue_empty;
static sem_t queue_full;

static thread_t* producer_threads[MAX_THREAD_COUNT];
static thread_t* consumer_threads[MAX_THREAD_COUNT];

static size_t active_threads;
static sem_t active_threads_guard;

static void register_thread(void) {
    sem_wait(&active_threads_guard);
    active_threads++;
    sem_post(&active_threads_guard);
}

static void unregister_thread(void) {
    sem_wait(&active_threads_guard);
    active_threads--;
    sem_post(&active_threads_guard);
}

static void* worker_producer(void* ignored) {
    register_thread();

    for (int i = 0; i < WORKER_ITEM_COUNT; i++) {
        // Try to deplete the quantum just before locking the semaphore.
        ktest_thread_busy_wait_random(WAIT_LOOPS_OUTSIDE_MAX);

        sem_wait(&queue_empty);
        // This is where we would lock the queue and add an item to it.
        sem_post(&queue_full);
        thread_yield();
    }

    unregister_thread();
    return NULL;
}

static void* worker_consumer(void* ignored) {
    register_thread();

    for (int i = 0; i < WORKER_ITEM_COUNT; i++) {
        // Try to deplete the quantum just before locking the semaphore.
        ktest_thread_busy_wait_random(WAIT_LOOPS_OUTSIDE_MAX);

        sem_wait(&queue_full);
        // This is where we would lock the queue and remove an item from it.
        sem_post(&queue_empty);
        thread_yield();
    }

    unregister_thread();
    return NULL;
}

static size_t spawn_threads(size_t count, const char* name, thread_entry_func_t entry, size_t total_count, thread_t* threads[]) {
    // Make sure not to exceed the capacity of the thread array.
    ktest_assert((total_count + count) <= MAX_THREAD_COUNT, "attempting to create too many %s threads", name);

    printk("Spawning %u %s threads...\n", count, name);
    for (size_t i = 0; i < count; i++) {
        ktest_thread_create_checked(&threads[total_count + i], entry, NULL, 0, name);
    }

    // Return the number of threads created (not the total).
    return count;
}

static void spawn_producers_scaled(size_t multiplier, size_t* total_count) {
    *total_count += spawn_threads(BASE_THREAD_COUNT * multiplier,
            "producer", worker_producer, *total_count, producer_threads);
}

static void spawn_consumers_scaled(size_t multiplier, size_t* total_count) {
    *total_count += spawn_threads(BASE_THREAD_COUNT * multiplier,
            "consumer", worker_consumer, *total_count, consumer_threads);
}

static void yield_cpu_to_workers() {
    for (int i = 0; i < WAIT_LOOPS; i++) {
        thread_yield();
    }
}

void kernel_test(void) {
    ktest_start("sem/race");

    // Active thread count is updated globally.
    active_threads = 0;
    errno_t err = sem_init(&active_threads_guard, 1);
    ktest_assert_errno(err, "sem_init(active_threads_guard)");

    err = sem_init(&queue_empty, QUEUE_SIZE);
    ktest_assert_errno(err, "sem_init(queue_empty)");
    err = sem_init(&queue_full, 0);
    ktest_assert_errno(err, "sem_init(queue_full)");

    // Accumulate worker thread counts locally.
    size_t producer_thread_count = 0;
    size_t consumer_thread_count = 0;

    spawn_producers_scaled(3, &producer_thread_count);
    spawn_consumers_scaled(1, &consumer_thread_count);
    yield_cpu_to_workers();

    spawn_producers_scaled(2, &producer_thread_count);
    spawn_consumers_scaled(2, &consumer_thread_count);
    yield_cpu_to_workers();

    spawn_producers_scaled(1, &producer_thread_count);
    spawn_consumers_scaled(3, &consumer_thread_count);
    yield_cpu_to_workers();

    ktest_assert(producer_thread_count == consumer_thread_count, "internal error, test not balanced");

    while (true) {
        dprintk("Queue: %pS %pS\n", &queue_empty, &queue_full);

        sem_wait(&active_threads_guard);
        size_t still_active = active_threads;
        sem_post(&active_threads_guard);

        printk("There are %u active threads ...\n", still_active);
        if (still_active == 0) {
            break;
        }

        yield_cpu_to_workers();
    }

    for (size_t i = 0; i < producer_thread_count; i++) {
        ktest_thread_join_checked(producer_threads[i], NULL);
    }
    for (size_t i = 0; i < consumer_thread_count; i++) {
        ktest_thread_join_checked(consumer_threads[i], NULL);
    }

    ktest_assert(sem_get_value(&queue_full) == 0, "queue still contains items");
    ktest_assert(sem_get_value(&queue_empty) == QUEUE_SIZE, "queue is not empty");

    ktest_passed();
}
