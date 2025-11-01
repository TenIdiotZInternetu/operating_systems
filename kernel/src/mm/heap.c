// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "adt/list.h"
#include <mm/heap.h>
#include <mm/self.h>

static const size_t PROBE_STEP = 4096;
static const uint64_t PROBE_TEST = 0xABABAB;
static const uint8_t MIN_BLOCK_SIZE = 8;

static heap_head_t heap_header;

typedef struct {
    bool is_free;
    size_t size;
    link_t link;
} block_head_t;

typedef block_head_t* block_tail_t;

#define BLOCK_OVERHEAD (sizeof(block_head_t) + sizeof(block_tail_t))

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

/**
 *  Writes a block tail structure at the given memory address.
 *
 * @param address The memory address where the block tail should be written.
 * @param p_block_head Pointer to the corresponding block header structure.
 */
static void _write_block_tail(uintptr_t address, block_head_t* p_block_head) {
    block_tail_t* p_block_tail = (block_tail_t*)(address);
    *p_block_tail = p_block_head;
}

/**
 * Initializes and writes a free memory block at the given address.
 *
 * @param address The starting memory address of the block's header.
 * @param size The size of the free block's usable memory area (in bytes).
 *
 * @return Pointer to the newly created block's header.
 */
static block_head_t* _create_free_block(uintptr_t address, size_t size) {
    block_head_t* p_free_block_head = (block_head_t*)(address);

    p_free_block_head->is_free = true;
    p_free_block_head->size = size;

    link_init(&p_free_block_head->link);

    uintptr_t free_block_tail_address = (uintptr_t)((uint8_t*)p_free_block_head + sizeof(block_head_t) + size);
    _write_block_tail(free_block_tail_address, p_free_block_head);

    return p_free_block_head;
}

/**
 * Initializes the free_blocks list and inserts one free block
 * of the size of entire memory minus size of the head and tail
 *
 * @return Pointer to the initialized free block
 */
static block_head_t* init_free_blocks_list(void) {
    list_init(&heap_header.free_blocks);

    uintptr_t first_free_block_address = (uintptr_t)((block_head_t*)kernel_get_image_end());
    size_t first_free_block_size = (get_memory_end() - kernel_get_image_end()) - BLOCK_OVERHEAD;

    block_head_t* p_first_block_head = _create_free_block(first_free_block_address, first_free_block_size);

    link_init(&p_first_block_head->link);

    list_append(&heap_header.free_blocks, &p_first_block_head->link);

    return p_first_block_head;
}

/** Heap initialization.
 *
 * Called once during system boot.
 */
void heap_init(void) {
    heap_header.start_address = kernel_get_image_end();
    heap_header.end_address = get_memory_end();
    block_head_t* init_block = init_free_blocks_list();

    heap_header.max_size = init_block->size;
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

/**
 * Checks if a requested allocation size fits into a free block, considering the
 * necessary overhead for block headers and tails.
 *
 * @param size_to_fit Requested size to allocate
 * @param free_size Current size of the free memory block
 *
 * @return true if the requested size fits within the free block (including
 *         metadata overhead), false otherwise.
 */
static bool fits_to_free_block(size_t size_to_fit, size_t free_size) {
    return size_to_fit <= (free_size - BLOCK_OVERHEAD);
}

/**
 * Iterates through heap's free blocks, returns first that fits
 *
 * @param size_to_alloc size of allocated data
 * @return Pointer to first free block head
 * @retval NULL No big enough block found
 */
static block_head_t* first_fit(size_t size_to_alloc) {
    list_foreach(heap_header.free_blocks, block_head_t, link, block) {
        if (block->is_free && (fits_to_free_block(size_to_alloc, block->size))) {
            return block;
        }
    }

    return NULL;
}

/**
 * Splits a free block into two, first used, second free.
 * Used block is removed from the list of free blocks, free block is inserted.
 *
 * @param free_block Pointer to free block to split
 * @param size_to_alloc Size of the created used block without head and tail
 */
static void split_block(block_head_t* free_block, size_t size_to_alloc) {
    const size_t used_block_required_size = size_to_alloc + BLOCK_OVERHEAD;

    size_t original_free_size = free_block->size;

    assert(used_block_required_size <= original_free_size + BLOCK_OVERHEAD);

    link_t* p_next_free_block_link = free_block->link.next;
    list_remove(&free_block->link);

    size_t remaining_space = original_free_size + BLOCK_OVERHEAD - used_block_required_size;

    free_block->is_free = false;

    // If remainder is too small to create a new free block, allocate the entire block to avoid fragmentation
    if (remaining_space < BLOCK_OVERHEAD + MIN_BLOCK_SIZE) {
        heap_header.free_size -= (original_free_size + BLOCK_OVERHEAD);
        return;
    }

    free_block->size = size_to_alloc;

    uintptr_t used_block_tail_address = (uintptr_t)((uint8_t*)free_block + sizeof(block_head_t) + size_to_alloc);
    _write_block_tail(used_block_tail_address, free_block);

    uintptr_t new_free_block_address = (uintptr_t)((uint8_t*)free_block + used_block_required_size);
    size_t new_free_block_remainig_size = remaining_space - BLOCK_OVERHEAD;
    block_head_t* p_free_block_head = _create_free_block(new_free_block_address, new_free_block_remainig_size);

    list_insert_before(&heap_header.free_blocks, p_next_free_block_link, &p_free_block_head->link);

    heap_header.free_size -= used_block_required_size;
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

    if (space_to_alloc + BLOCK_OVERHEAD > heap_header.free_size) {
        return NULL;
    }

    block_head_t* found_block = first_fit(space_to_alloc);

    if (found_block == NULL) {
        return NULL;
    }

    split_block(found_block, space_to_alloc);

    // Return first address after head
    void* alloc_ptr = (void*)(found_block + 1);
    return alloc_ptr;
}

/**
 * Checks if the block to the left is free. If yes the two blocks are merged
 * into one free block. If not, the pass block is returned.
 *
 * @param used_block Block to be coalesced
 * @return Pointer to the resulting free block
 */
static block_head_t* coalesce_left(block_head_t* freed_block) {
    // TODO *
    // Get to prev block through its tail
    return NULL;
}

/**
 * Checks if the block to the right is free. If yes the two blocks are merged
 * into one free block.
 *
 * @param used_block Block to be coalesced
 */
static void coalesce_right(block_head_t* freed_block) {
    // TODO *
    // Get to next block by adding the address and size of the freed block
}

/**
 * Inserts freed block back to the free_blocks list
 *
 * @param freed_block Block to insert
 */
static void insert_freed_block(block_head_t* freed_block) {
    // TODO *
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
    block_head_t* freed_block = (block_head_t*)ptr - 1;
    freed_block->is_free = true;

    freed_block = coalesce_left(freed_block);
    coalesce_right(freed_block);
    insert_freed_block(freed_block);
}

heap_head_t get_heap_info() {
    return heap_header;
}
