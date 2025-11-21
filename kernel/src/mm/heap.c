// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "adt/list.h"
#include <mm/frame.h>
#include <mm/heap.h>
#include <mm/self.h>

static heap_head_t heap_header;

#define ALIGNED_HEAD_SIZE align(sizeof(block_head_t))
#define ALIGNED_TAIL_SIZE align(sizeof(block_tail_t))

#define BLOCK_OVERHEAD (ALIGNED_HEAD_SIZE + ALIGNED_TAIL_SIZE)

static size_t align(size_t size_to_alloc) {
    if (size_to_alloc % MIN_BLOCK_SIZE == 0) {
        return size_to_alloc;
    } else {
        size_t blocks_to_alloc = (size_to_alloc / MIN_BLOCK_SIZE);
        return MIN_BLOCK_SIZE * (blocks_to_alloc + 1);
    }
}

uintptr_t get_heap_start_address(void) {
    return heap_header.start_address;
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
 * @param size The size of the free block's usable memory area.
 *
 * @return Pointer to the newly created block's header.
 */
static block_head_t* _create_free_block(uintptr_t header_address, size_t size) {
    block_head_t* p_free_block_head = (block_head_t*)(header_address);
    p_free_block_head->is_free = true;
    p_free_block_head->size = size;
    link_init(&p_free_block_head->link);

    uintptr_t free_block_tail_address = header_address + ALIGNED_HEAD_SIZE + size;
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

    uintptr_t first_free_block_address = heap_header.start_address;
    size_t first_free_block_size = (get_memory_end() - heap_header.start_address) - BLOCK_OVERHEAD;
    block_head_t* p_first_block_head = _create_free_block(first_free_block_address, first_free_block_size);
    list_append(&heap_header.free_blocks, &p_first_block_head->link);

    return p_first_block_head;
}

/** Heap initialization.
 *
 * Called once during system boot.
 */
void heap_init(void) {
    uintptr_t memory_start = get_memory_start();
    uintptr_t memory_end = get_memory_end_from(memory_start);

    uintptr_t heap_start = memory_start + (uintptr_t)(get_frame_region_size());

    update_free_frame_count(heap_start);

    heap_header.start_address = heap_start;
    heap_header.end_address = memory_end;

    block_head_t* init_block = init_free_blocks_list();

    heap_header.max_size = init_block->size;
    heap_header.free_size = heap_header.max_size;
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
        if (block->is_free && (size_to_alloc <= block->size)) {
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
static void split_block(block_head_t* free_block_head, size_t size_to_alloc) {
    size_t original_free_size = free_block_head->size;

    link_t* p_next_free_block_link = free_block_head->link.next;
    printk("aaaaa");
    list_remove(&free_block_head->link);
    printk("bbbbbb");

    size_t used_block_total_size
            = size_to_alloc + BLOCK_OVERHEAD;
    size_t remaining_space = original_free_size + BLOCK_OVERHEAD - used_block_total_size;

    free_block_head->is_free = false;

    // If remainder is too small to create a new free block, allocate the entire block to avoid fragmentation
    if (remaining_space < BLOCK_OVERHEAD + MIN_BLOCK_SIZE) {
        heap_header.free_size -= original_free_size;
        return;
    }

    free_block_head->size = size_to_alloc;
    assert(free_block_head->size % MIN_BLOCK_SIZE == 0);

    uintptr_t used_block_tail_address = (uintptr_t)free_block_head + ALIGNED_HEAD_SIZE + size_to_alloc;
    _write_block_tail(used_block_tail_address, free_block_head);

    uintptr_t new_free_block_address = used_block_tail_address + ALIGNED_TAIL_SIZE;
    size_t new_free_block_data_size = remaining_space - BLOCK_OVERHEAD;

    block_head_t* p_free_block_head = _create_free_block(new_free_block_address, new_free_block_data_size);

    list_insert_before(&heap_header.free_blocks, p_next_free_block_link, &p_free_block_head->link);

    heap_header.free_size -= used_block_total_size;
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

    block_head_t* found_block = first_fit(space_to_alloc);

    if (found_block == NULL) {
        return NULL;
    }

    split_block(found_block, space_to_alloc);

    void* alloc_ptr = (void*)((uint8_t*)found_block + ALIGNED_HEAD_SIZE);
    return alloc_ptr;
}

/**
 *
 * Changes the head of the left block and the tail of the right block,
 * so that the right block is merged into the left
 *
 * @param left_block Pointer to the head of the block we are merging into
 * @param right_block Pointer to the head of the block we will merge into the left block
 */
static void coalesce(block_head_t* left_block, block_head_t* right_block) {
    left_block->size += BLOCK_OVERHEAD + right_block->size;
    block_tail_t* right_tail = (block_tail_t*)((uint8_t*)right_block + ALIGNED_HEAD_SIZE + right_block->size);
    *right_tail = left_block;
    heap_header.free_size += BLOCK_OVERHEAD;
}

/**
 * Checks if the block to the left is free. If yes the two blocks are merged
 * into one free block. If not, the pass block is returned.
 *
 * @param used_block Block to be coalesced
 * @return Pointer to the resulting free block
 */
static block_head_t* coalesce_left(block_head_t* freed_block) {
    if ((uintptr_t)freed_block <= heap_header.start_address) {
        return freed_block;
    }
    
    // Get to prev block through its tail
    block_tail_t* prev_tail = (block_tail_t*)((uint8_t*)freed_block - ALIGNED_TAIL_SIZE);
    block_head_t* prev_head = *prev_tail;

    if (prev_head->is_free) {
        coalesce(prev_head, freed_block);
        return prev_head;
    } else {
        return freed_block;
    }
}

/**
 * Checks if the block to the right is free. If yes the two blocks are merged
 * into one free block.
 *
 * @param used_block Block to be coalesced
 */
static void coalesce_right(block_head_t* freed_block) {
    uintptr_t next_address = (uintptr_t)freed_block + BLOCK_OVERHEAD + freed_block->size;
    
    if (next_address >= heap_header.end_address) {
        return;
    }
    
    // Get to next block by adding the address and size of the freed block
    block_head_t* next_head = (block_head_t*)next_address;

    if (next_head->is_free) {
        coalesce(freed_block, next_head);
        list_remove(&next_head->link);
    }
}

/**
 * Inserts freed block back to the free_blocks list
 *
 * @param freed_block Block to insert
 */
static void insert_freed_block(block_head_t* freed_block) {
    list_prepend(&heap_header.free_blocks, &freed_block->link);
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
    block_head_t* freed_block_head = (block_head_t*)((uint8_t*)ptr - ALIGNED_HEAD_SIZE);
    freed_block_head->is_free = true;
    heap_header.free_size += freed_block_head->size;
    block_head_t* new_head = freed_block_head;

    bool is_first_block = (uintptr_t)freed_block_head == heap_header.start_address;
    if (!is_first_block) {
        new_head = coalesce_left(freed_block_head);
    }
    bool coalesced_left = new_head != freed_block_head;

    bool is_last_block = (uintptr_t)((uint8_t*)new_head + BLOCK_OVERHEAD + new_head->size) == heap_header.end_address;
    if (!is_last_block) {
        coalesce_right(new_head);
    }

    if (!coalesced_left) {
        insert_freed_block(new_head);
    }
}

heap_head_t get_heap_info() {
    return heap_header;
}

void print_heap() {
    printk("\
{ HEAP:\n \
    used: %u/%u,\n \
    free: %u,\n \
    range: %p-%p\n",
            heap_header.max_size - heap_header.free_size,
            heap_header.max_size,
            heap_header.free_size,
            heap_header.start_address,
            heap_header.end_address);

    printk("\n  BLOCKS:\n");

    uintptr_t block_address = heap_header.start_address;
    while (block_address < heap_header.end_address) {
        printk("    %pA\n", block_address + ALIGNED_HEAD_SIZE);
        size_t block_size = ((block_head_t*)block_address)->size;
        block_address += block_size + BLOCK_OVERHEAD;
    }

    printk("\n  FREE BLOCKS LIST:\n");
    list_foreach(heap_header.free_blocks, block_head_t, link, it) {
        printk("    %pA\n", (uintptr_t)it + ALIGNED_HEAD_SIZE);
    }

    printk("}\n");
}