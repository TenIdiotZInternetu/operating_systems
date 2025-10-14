// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <mm/heap.h>
#include <mm/self.h>

#define FREE_MEM_NEXT_TRY 4096
#define MIN_HEAP_BLOCK 16

typedef struct {
    uintptr_t end_address;
    uintptr_t next_ptr;
} heapHeader_t;

static heapHeader_t heap_header;

// tools/tester.py suite suites/m02-heap-base.txt

/** Heap initialization.
 *
 * Called once during system boot.
 */
void heap_init(void) {
    uint64_t test_write = 25;
    uintptr_t start = kernel_get_image_end();
    heap_header.next_ptr = start;
    volatile uint64_t* current = (uint64_t*)start + FREE_MEM_NEXT_TRY / sizeof(test_write);
    *current = test_write;

    while (*current == test_write) {
        current += FREE_MEM_NEXT_TRY / sizeof(test_write);
        *current = test_write;
    }

    heap_header.end_address = (uintptr_t)current - FREE_MEM_NEXT_TRY;
}

static size_t align(size_t size_to_alloc) {
    size_t aligned;
    if (size_to_alloc % MIN_HEAP_BLOCK == 0) {
        aligned = size_to_alloc;
    }
    else {
        aligned = MIN_HEAP_BLOCK * (size_to_alloc / MIN_HEAP_BLOCK) + MIN_HEAP_BLOCK;
    }
    return aligned;
}

/** Allocate size bytes on heap.
 *
 * The behavior when size is zero is not defined and none of the provided
 * tests will ever call kmalloc with 0 as the parameter. The implementation may
 * choose to return NULL or a unique pointer for such call (i.e., follow the
 * C standard). Note that causing a kernel panic or not checking for such
 * case at all is also a valid option.
 *
 * @param size Amount of memory to allocate.
 * @return Pointer to allocated memory.
 * @retval NULL Out of memory.
 */
void* kmalloc(size_t size) {
    size_t space_to_alloc = align(size);
    uintptr_t new_end = heap_header.next_ptr + space_to_alloc;
    void* alloc_ptr = NULL;

    if (new_end <= heap_header.end_address) {
        alloc_ptr = (void*)heap_header.next_ptr;
        heap_header.next_ptr += space_to_alloc;
    }
    return alloc_ptr;
}

/** Free memory previously allocated by kmalloc.
 *
 * The behavior when ptr is NULL is not defined and none of the provided
 * tests will ever call kfree with NULL as the parameter. Note that accepting
 * NULL as a parameter follows the C standard behavior, i.e. everything that
 * malloc can return can be passed to free. In the context of a kernel,
 * aborting the execution on receiving NULL is also an option as that might
 * signal an abnormal situation in a different module.
 *
 * @param ptr Pointer to memory to be freed.
 */
void kfree(void* ptr) {
}
