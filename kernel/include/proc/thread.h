// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_THREAD_H
#define _PROC_THREAD_H

#include <adt/list.h>
#include <drivers/sv32.h>
#include <errno.h>
#include <exc.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <proc/context.h>
#include <proc/process.h>

/** Thread stack size.
 *
 * Set quite liberally because stack overflows are notoriously
 * difficult to debug (and difficult to detect too).
 */
#define THREAD_STACK_SIZE 4096

#define THREAD_STACK_SIZE_IN_FRAMES THREAD_STACK_SIZE / FRAME_SIZE

/** Max length of a thread name (excluding the terminating zero). */
#define THREAD_NAME_MAX_LENGTH 31

#define THREAD_USERSPACE_FLAG 1

/* Forward declaration to prevent cyclic header inclusions. */
typedef struct as as_t;

/** Thread entry function (similar to pthreads). */
typedef void* (*thread_entry_func_t)(void*);

/** Information about any existing thread. */
typedef struct thread thread_t;

typedef enum {
    READY,
    SUSPENDED,
    FINISHED,
    KILLED
} thread_state_t;

struct thread {
    char name[THREAD_NAME_MAX_LENGTH + 1];
    thread_entry_func_t entry_func;
    context_t context;
    thread_state_t state;
    uintptr_t stack_location;
    void* return_value;
    void* first_arg;
    thread_t* joined_by_thread;
    link_t link;
    as_t* address_space;
    process_t* process;
    bool is_initialized;
    unsigned int flags;
};

void threads_init(void);

errno_t thread_create(thread_t** thread, thread_entry_func_t entry, void* data, unsigned int flags, const char* name);
errno_t thread_create_new_as(thread_t** thread, thread_entry_func_t entry, void* data, unsigned int flags, const char* name, size_t as_size);

thread_t* thread_get_current(void);
void thread_yield(void);
void thread_suspend(void);
void thread_finish(void* retval) __attribute__((noreturn));
void _thread_set_name(thread_t* thread, const char* name);
bool thread_has_finished(thread_t* thread);

errno_t thread_wakeup(thread_t* thread);
errno_t thread_join(thread_t* thread, void** retval);
void thread_switch_to(thread_t* thread);

as_t* thread_get_as(thread_t* thread);
errno_t thread_kill(thread_t* thread);

const char* thread_state_str(thread_t* thread);

#endif
