// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_NP_PROC_H
#define _LIBC_NP_PROC_H

#include <np/types.h>
#include <stdbool.h>
#include <stddef.h>

#define NP_NULL_AREA_SIZE (2 * 4096)

/** Information about running process.
 *
 * See np_proc_info_get for details.
 */
typedef struct {
    unative_t id;
    size_t virt_mem_size;
    size_t total_ticks;
} np_proc_info_t;

bool np_proc_info_get(np_proc_info_t* info);

/** End of application code (see _kernel_end). */
extern uint8_t _app_end[0];

#endif
