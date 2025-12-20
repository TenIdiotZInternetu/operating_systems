// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_AS_H
#define _MM_AS_H

#include <errno.h>
#include <proc/thread.h>
#include <types.h>

/*
 * Setup amount of ASIDS that are actually supported by the hardware.
 *
 * We use a a simplification here: MSIM supports full 9 bits of ASIDs and
 * do not require our kernel to detect that.
 *
 * To speed up some of the tests we set this constant from tester.py so that
 * the implementation uses less values and thus more quickly recycles the
 * ASID numbers.
 */
#ifndef HARDWARE_ASIDS_MAX
#define HARDWARE_ASIDS_MAX 512
#endif

#define PAGE_OFFSET_BITS 12
#define PAGE_SIZE 4096
#define PTE_SIZE_BYTES 4
#define PTE_SIZE_BITS (PTE_SIZE_BYTES * 8)
#define PAGES_PER_TABLE (PAGE_SIZE / PTE_SIZE_BYTES)
#define MEGAPAGE_SIZE (PAGES_PER_TABLE * PAGE_SIZE)

#define PAGE_NULL_COUNT 2
#define AS_STACK_ADDRESS (1 << 13) // 8kB

#define NULL_PAGES_REGION_SIZE (PAGE_NULL_COUNT * PAGE_SIZE)

#define PAGE_SIZE_ALIGNMENT_MASK ~(PAGE_SIZE - 1)

/* forward declaration */
typedef struct thread thread_t;

typedef struct as {
    size_t asid;
    uintptr_t root_table_addr;
    size_t using_threads;
    size_t size;
} as_t;

void as_init(void);
as_t* as_create(size_t size, unsigned int flags);
uintptr_t as_alloc_page(as_t* as, uintptr_t virtual_addr, size_t count);
void as_destroy(as_t* as);

void as_add_thread(as_t* as, thread_t* thread);
void as_remove_thread(thread_t* thread);

size_t as_get_size(as_t* as);
uintptr_t as_get_root_page_table(as_t* as);

static inline size_t addr_to_ppn(uintptr_t address) {
    return address >> PAGE_OFFSET_BITS;
}

static inline bool is_null_pages_access(uintptr_t address) {
    return address < (NULL_PAGES_REGION_SIZE);
}

static inline bool is_address_beyond_address_space(uintptr_t address, as_t *address_space) {
    return address >= (NULL_PAGES_REGION_SIZE + address_space->size);
}

bool is_kernel_pages_access(uintptr_t address);

#endif
