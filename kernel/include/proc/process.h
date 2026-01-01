// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_PROCESS_H
#define _PROC_PROCESS_H

#include <errno.h>
#include <mm/as.h>
#include <proc/thread.h>
#include <types.h>

/** Virtual address of the entry point to userspace application. */
#define PROCESS_ENTRY_POINT 0x00004000

/** Virtual address where the application binary is mounted in MSIM. */
#define PROCESS_IMAGE_START 0xB8000000

/** Maximum size of the application binary.
 *
 * This specifies the safe upper bound of the application binary size. If you
 * need to map the application binary to a specific address (e.g., different
 * from hard-wired identity mapping), you can safely map this amount of
 * memory.
 *
 * The PROCESS_IMAGE_SIZE specifies the actual image size to copy.
 */
#define PROCESS_IMAGE_MAX_SIZE (1024 * 1024 * 4)

/** Actual size of the application binary. */
#define PROCESS_IMAGE_SIZE (1024 * 128)

#define USERSPACE_STACK_PAGES 2
#define USERSPACE_STACK_REGION_SIZE (USERSPACE_STACK_PAGES * PAGE_SIZE)
#define USERSPACE_STACK_TOP (NULL_PAGES_REGION_SIZE + USERSPACE_STACK_REGION_SIZE)

#ifndef PROCESS_MEMORY_SIZE
/** Amount of virtual memory to give to the userspace process. */
#define PROCESS_MEMORY_SIZE (PROCESS_IMAGE_SIZE * 2)
#endif

#if PROCESS_MEMORY_SIZE < PROCESS_IMAGE_SIZE
#error "Cannot give less memory than image size!"
#endif

typedef struct thread thread_t;

/** Information about existing process. */
typedef struct {
    thread_t* user_thread;
    uintptr_t image_start;
    size_t image_size;
    int exit_status;
} process_t;

errno_t process_create(process_t** process, uintptr_t image_location, size_t image_size, size_t process_memory_size);
errno_t process_join(process_t* process, int* exit_status);

#endif
