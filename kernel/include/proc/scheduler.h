// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_SCHEDULER_H
#define _PROC_SCHEDULER_H

#include <adt/list.h>
#include <proc/thread.h>

typedef struct {
    thread_t* current_thread;
    thread_t* thread_to_launch;
    list_t ready_threads;
} scheduler_t;

void scheduler_init(void);
thread_t* scheduler_get_current_thread(void);
void scheduler_set_current_thread(thread_t* thread_to_set);
thread_t* scheduler_get_thread_to_launch(void);
void scheduler_add_ready_thread(thread_t* id);
void scheduler_remove_thread(thread_t* id);
void scheduler_schedule_next(void);

#endif
