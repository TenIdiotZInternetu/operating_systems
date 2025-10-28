// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "adt/list.h"
#include <mm/heap.h>
#include <mm/self.h>

static const size_t PROBE_STEP = 4096;
static const uint64_t PROBE_TEST = 0xABABAB;
static const uint8_t MIN_BLOCK_SIZE = 8;

static uint32_t mock_heap[MOCK_HEAP_SIZE / sizeof(uint32_t)];

static heap_head_t heap_header;

typedef struct {
    bool is_free;
    size_t size;
    link_t link;
} block_head_t;

typedef block_head_t* block_tail_t;


static uintptr_t get_memory_end(void)
{
    uintptr_t start = (uintptr_t)kernel_get_image_end();
    volatile uint64_t *current = (volatile uint64_t *)(start + PROBE_STEP);

    *current = PROBE_TEST;

    while (*current == PROBE_TEST) {
        current = (volatile uint64_t *)((uintptr_t)current + PROBE_STEP);
        *current = PROBE_TEST;
    }

    return (uintptr_t)current - PROBE_STEP;
}


/**
* Initializes the free_blocks list and inserts one free block
* of the size of entire memory minus size of the head and tail
*
* @return Pointer to the initialized free block
*/
static block_head_t* init_free_blocks_list(void) {

    // Write head
    block_head_t *p_first_block_head = kernel_get_image_end();
    p_first_block_head->is_free = true;
    p_first_block_head->size = (get_memory_end() - kernel_get_image_end()) - get_block_head_tail_size();

    // Write tail
    block_tail_t *p_first_block_tail = (block_tail_t *)(
        (uint8_t *)p_first_block_head + p_first_block_head->size
    );
    *p_first_block_tail = p_first_block_head;

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
 * Get the total metadata size for a memory block
 *
 * @return Total size of the block header and tail.
 */
static size_t get_block_head_tail_size() {
    return sizeof(block_head_t) + sizeof(block_tail_t);
}

/**
 * Checks if a requested allocation size fits into a free block, considering the
 * necessary overhead for block headers and tails.
 * 
 * @param size_to_fit: Requested size to allocate
 * @param free_size: Current size of the free memory block
 *
 * @return true if the requested size fits within the free block (including
 *         metadata overhead), false otherwise.
 */
static bool fits_to_free_block(size_t size_to_fit, size_t free_size) {
    return size_to_fit <= (free_size - get_block_head_tail_size());
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
            return &block;
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
static void split_block(block_head_t *free_block, size_t size_to_alloc)
{
    const size_t block_overhead = get_block_head_tail_size();
    const size_t used_block_required_size = size_to_alloc + block_overhead;

    assert(used_block_required_size <= free_block->size);

    link_t *next_free_block_link = free_block->link.next;
    list_remove(&free_block->link);

    // Write used block tail
    block_tail_t *p_used_block_tail = (block_tail_t *)(
        (uint8_t *)free_block + used_block_required_size - sizeof(block_tail_t)
    );
    *p_used_block_tail = free_block;

    // If there won't be space for the new free block, return
    size_t remaining_free_size = free_block->size - used_block_required_size;
    if (remaining_free_size <= block_overhead)
        return;

    // Initialize the new free block
    block_head_t *p_new_free_block_head = (block_head_t *)(
        (uint8_t *)free_block + used_block_required_size
    );
    p_new_free_block_head->is_free = true;
    p_new_free_block_head->size = remaining_free_size - block_overhead;
    link_init(&p_new_free_block_head->link);
    
    // Write free block tail
    block_tail_t *p_new_free_block_tail = (block_tail_t *)(
        (uint8_t *)p_new_free_block_head + p_new_free_block_head->size
    );
    *p_new_free_block_tail = p_new_free_block_head;

    // Insert new free block into free list
    list_insert_before(&heap_header.free_blocks, next_free_block_link, &p_new_free_block_head->link);

    // Update heap free size
    heap_header.free_size -= used_block_required_size;
    heap_header.free_size += p_new_free_block_head->size;
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

    if (space_to_alloc + get_block_head_tail_size() > heap_header.free_size) {
        return NULL;
    }

    block_head_t* found_block = first_fit(space_to_alloc);

    if (found_block == NULL) {
        return NULL;
    }

    split_block(found_block, space_to_alloc);

    // Return first address after head
    void *alloc_ptr = (void*)(found_block + 1);
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
