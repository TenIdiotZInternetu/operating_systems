// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <proc/scheduler.h>

static scheduler_t scheduler;

/**
 * Checks if the scheduler has been initialized
 * @return true if the scheduler has been initialized
 */
static bool scheduler_is_init() {
    // TODO
    return true;
}

/** Initialize support for scheduling.
 *
 * Called once at system boot.
 */
void scheduler_init(void) {
    list_init(&scheduler.ready_threads);
    scheduler.current_thread = NULL;
}

thread_t* scheduler_get_current_thread(void) {
    return scheduler.current_thread;
}

/** Marks given thread as ready to be executed.
 *
 * It is expected that this thread would be added at the end
 * of the queue to run in round-robin fashion.
 *
 * @param thread Thread to make runnable.
 */
void scheduler_add_ready_thread(thread_t* thread) {
    assert(scheduler_is_init());

    list_append(&scheduler.ready_threads, &thread->link);
}

/** Removes given thread from scheduling.
 *
 * Expected to be called when thread is suspended, terminates execution
 * etc.
 *
 * @param thread Thread to remove from the queue.
 */
void scheduler_remove_thread(thread_t* thread) {
    assert(scheduler_is_init());
    assert(link_is_connected(&thread->link));

    list_remove(&thread->link);
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {
    /**
     * TODO: Call thread_switch_context(current_thread)
     *
     * Before that, change the current_thread to the next thread in the list.
     * Careful about the list head.
     *
     * You need to get the context of the switched-from thread into the function.
     * Feel free to add fields to scheduler_t, and adjust the implementation of thread_switch_context()
     */
}
