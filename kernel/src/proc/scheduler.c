// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <proc/scheduler.h>
#include <debug/code.h>

static scheduler_t scheduler;

/**
 * Checks if the scheduler has been initialized
 * @return true if the scheduler has been initialized
 */
static bool scheduler_is_init() {

    link_t *ready_threads_head = &scheduler.ready_threads.head;
    
    return (ready_threads_head->next == ready_threads_head && ready_threads_head->prev == ready_threads_head);
}

/** Initialize support for scheduling.
 *
 * Called once at system boot.
 */
void scheduler_init(void) {
    list_init(&scheduler.ready_threads);
    list_init(&scheduler.idle_threads);
}

thread_t* scheduler_get_current_thread(void) {
    assert(scheduler_is_init());
    assert(!list_is_empty(&scheduler.ready_threads));
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
    assert(scheduler_is_init());
    assert(link_is_connected(&thread->link));

    list_remove(&thread->link);
    list_append(&scheduler.idle_threads, &thread->link);
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {
    /**
     * TODO: Call thread_switch_to(next thread)
     *
     * You need to get the context of the switched-from thread into the function.
     * Feel free to add fields to scheduler_t, and adjust the implementation of thread_switch_context()
     *
     * It should be safe to change current_thread value after the context switch.
     *
     * Move the current thread to the end of the queue before switching.
     */

    thread_t *p_cur_thread = scheduler_get_current_thread();

    thread_suspend();

    scheduler_add_ready_thread(p_cur_thread);

    thread_t *p_next_thread = scheduler_get_thread_to_launch();

    //scheduler_set_current_thread(p_next_thread);

    thread_switch_to(p_next_thread);

    scheduler_set_current_thread(p_next_thread);

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