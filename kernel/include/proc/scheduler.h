// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_SCHEDULER_H
#define _PROC_SCHEDULER_H

#include <adt/list.h>
#include <proc/thread.h>

typedef struct {
    list_t ready_threads;
    list_t idle_threads;

    thread_t* current_thread;
    thread_t* previous_thread;

    bool is_initialized;
} scheduler_t;

void scheduler_init(void);
thread_t* scheduler_get_current_thread(void);
thread_t* scheduler_get_previous_thread(void);
void scheduler_add_ready_thread(thread_t* thread);
void scheduler_remove_thread(thread_t* thread);
void scheduler_schedule_next(void);

void print_scheduler(void);

#endif
