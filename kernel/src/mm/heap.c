// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <mm/heap.h>
#include <mm/self.h>

static const size_t PROBE_STEP = 4096;
static const uint64_t PROBE_TEST = 0xABABAB;
static const uint8_t MIN_BLOCK_SIZE = 8;

static heap_head_t heap_header;

static uintptr_t get_memory_end() {
    uintptr_t start = kernel_get_image_end();
    volatile uint64_t* current = (uint64_t*)start + PROBE_STEP / sizeof(PROBE_TEST);
    *current = PROBE_TEST;

    while (*current == PROBE_TEST) {
        current += PROBE_STEP / sizeof(PROBE_TEST);
        *current = PROBE_TEST;
    }

    return (uintptr_t)current - PROBE_STEP;
}

// tools/tester.py suite suites/m02-heap-base.txt

/** Heap initialization.
 *
 * Called once during system boot.
 */
void heap_init(void) {
    heap_header.start_address = kernel_get_image_end();
    heap_header.next_ptr = heap_header.start_address;
    heap_header.end_address = get_memory_end();
    heap_header.max_size = heap_header.end_address - heap_header.start_address;
    heap_header.free_size = heap_header.max_size;
}

static size_t align(size_t size_to_alloc) {
    if (size_to_alloc % MIN_BLOCK_SIZE == 0) {
        return size_to_alloc;
    } else {
        size_t blocks_to_alloc = (size_to_alloc / MIN_BLOCK_SIZE);
        return MIN_BLOCK_SIZE * (blocks_to_alloc + 1);
    }
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

    if (space_to_alloc > heap_header.free_size) {
        return NULL;
    }

    void* alloc_ptr = (void*)heap_header.next_ptr;
    heap_header.next_ptr += space_to_alloc;
    heap_header.free_size -= space_to_alloc;
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

heap_head_t get_heap_info() {
    return heap_header;
}
