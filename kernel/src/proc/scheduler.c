// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <debug/code.h>
#include <proc/scheduler.h>

static scheduler_t scheduler;

/**
 * Checks if the scheduler has been initialized
 * @return true if the scheduler has been initialized
 */
static bool scheduler_is_init() {
    return scheduler.is_initialized;
}

/** Initialize support for scheduling.
 *
 * Called once at system boot.
 */
void scheduler_init(void) {
    list_init(&scheduler.ready_threads);
    list_init(&scheduler.idle_threads);

    scheduler.current_thread = NULL;

    scheduler.is_initialized = true;
}

thread_t* scheduler_get_current_thread(void) {
    assert(scheduler_is_init());
    return scheduler.current_thread;
}

/**
 * Returns the previously running thread before the last context switch.
 */
thread_t* scheduler_get_previous_thread(void) {
    assert(scheduler_is_init());
    return scheduler.previous_thread;
}

static void _remove_from_queue(thread_t* thread) {
    if (link_is_connected(&thread->link)) {
        list_remove(&thread->link);
    }
}

/**
 * Moves the given thread to either the ready or idle list based on whether it is ready.
 */
static void _move_to_list(thread_t* thread, bool ready) {
    list_t* p_list_to_append = ready ? &scheduler.ready_threads : &scheduler.idle_threads;
    list_append(p_list_to_append, &thread->link);
}

static bool _has_ready_threads(void) {
    return !list_is_empty(&scheduler.ready_threads);
}

/**
 * Determines if a thread is valid and in the READY state.
 */
static bool _can_thread_run(thread_t* thread) {
    return (thread != NULL && thread->state == READY);
}

/**
 * Saves the source and destination threads for an upcoming context switch.
 */
static void _save_for_context_switch(thread_t* thread_from, thread_t* thread_to) {
    scheduler.previous_thread = thread_from;
    scheduler.current_thread = thread_to;
}

/**
 * Retrieves the first thread on the ready list.
 */
static thread_t* scheduler_get_thread_to_launch(void) {
    return list_item(scheduler.ready_threads.head.next, thread_t, link);
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
    assert(!thread_has_finished(thread));

    _remove_from_queue(thread);

    _move_to_list(thread, true);
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

    _remove_from_queue(thread);

    _move_to_list(thread, false);
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {

    thread_t* p_cur_thread = scheduler.current_thread;

    bool is_cur_ready = _can_thread_run(p_cur_thread);

    if (!_has_ready_threads()) {
        if (is_cur_ready) {
            return; // Nowhere to switch
        } else {
            panic("No threads to schedule.");
        }
    }

    if (is_cur_ready) {
        _remove_from_queue(p_cur_thread);
        _move_to_list(p_cur_thread, true);
    }

    thread_t* p_next_thread = scheduler_get_thread_to_launch();
    _remove_from_queue(p_next_thread);

    // Save for context switch
    _save_for_context_switch(p_cur_thread, p_next_thread);

    thread_switch_to(p_next_thread);
}

void print_scheduler(void) {
    uint16_t ready_thread_count = 0;
    uint16_t idle_thread_count = 0;

    printk("{ SCHEDULER\n");
    printk("    READY THREADS:\n");
    list_foreach(scheduler.ready_threads, thread_t, link, it) {
        printk("    %pT\n", it);
        ready_thread_count++;
    };

    printk("\n");
    printk("    IDLE THREADS:\n");
    list_foreach(scheduler.idle_threads, thread_t, link, it) {
        printk("    %pT\n", it);
        idle_thread_count++;
    }

    printk("\n");
    printk("    Thread count: %u/%u (ready/idle)\n", ready_thread_count, idle_thread_count);
    printk("    Total count: %u\n", ready_thread_count + idle_thread_count);
    printk("}\n");
}
