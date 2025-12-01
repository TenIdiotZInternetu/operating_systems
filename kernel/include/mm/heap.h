// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_HEAP_H
#define _MM_HEAP_H

#include "adt/list.h"
#include <types.h>

typedef struct {
    size_t free_size;
    size_t max_size;
    uintptr_t start_address;
    uintptr_t end_address;
    list_t free_blocks;
} heap_head_t;

typedef struct {
    size_t magic;
    bool is_free;
    size_t size;
    link_t link;
} block_head_t;

typedef block_head_t* block_tail_t;

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
heap_head_t get_heap_info(void);
void print_heap(void);

uintptr_t get_heap_start_address(void);

#endif
