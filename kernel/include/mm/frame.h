// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_FRAME_H
#define _MM_FRAME_H

#include <errno.h>
#include <types.h>

#define FRAME_SIZE 4096
#define MIN_BLOCK_SIZE 8

#define FRAME_KSEG0_SIZE (512 * 1024 * 1024)
#define FRAME_COUNT_MAX (FRAME_KSEG0_SIZE / FRAME_SIZE)

#define FRAME_REGION_MEMORY_DIVISOR 2

typedef struct {
    size_t free_frame_count;
} frame_allocator_t;

void frame_init(void);
errno_t frame_alloc(size_t count, uintptr_t* phys);
errno_t frame_free(size_t count, uintptr_t phys);

void update_free_frame_count(uintptr_t start_address);

size_t get_frame_region_size(void);

#endif
