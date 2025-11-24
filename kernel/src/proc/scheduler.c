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
static bool _scheduler_is_init() {
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
    scheduler.previous_thread = NULL;

    scheduler.is_initialized = true;

    timer_interrupt_after(KERNEL_SCHEDULER_QUANTUM);
}

thread_t* scheduler_get_current_thread(void) {
    assert(_scheduler_is_init());
    return scheduler.current_thread;
}

/**
 * Returns the previously running thread before the last context switch.
 */
thread_t* scheduler_get_previous_thread(void) {
    assert(_scheduler_is_init());
    return scheduler.previous_thread;
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
 * Retrieves the first thread on the ready list.
 */
static thread_t* _scheduler_get_thread_to_run(void) {
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
    assert(_scheduler_is_init());
    assert(!thread_has_finished(thread));

    if (link_is_connected(&thread->link)) {
        list_remove(&thread->link);
    }
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
    assert(_scheduler_is_init());
    assert(link_is_connected(&thread->link));

    list_remove(&thread->link);
    list_append(&scheduler.idle_threads, &thread->link);
}

static void _update_scheduller(void) {
    thread_t* p_cur_thread = scheduler.current_thread;
    bool is_cur_ready = _can_thread_run(p_cur_thread);

    if (!_has_ready_threads()) {
        panic("No threads to schedule.");
    }

    if (is_cur_ready) {
        list_rotate(&scheduler.ready_threads);
    }

    thread_t* p_next_thread = _scheduler_get_thread_to_run();

    scheduler.previous_thread = p_cur_thread;
    scheduler.current_thread = p_next_thread;
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {
    _update_scheduller();

    thread_t* p_next_thread = _scheduler_get_thread_to_run();

    thread_switch_to(p_next_thread);
}

void scheduler_schedule_next_after_interrupt(void) {
    _update_scheduller();
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
