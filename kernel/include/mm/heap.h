// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_HEAP_H
#define _MM_HEAP_H

#include <types.h>

typedef struct {
    size_t free_size;
    size_t max_size;
    uintptr_t start_address;
    uintptr_t end_address;
    uintptr_t next_ptr;
} heap_info_t;

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
heap_info_t get_heap_info(void);


#endif
