// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/bits.h>
#include <debug.h>
#include <mm/frame.h>
#include <types.h>

/**
 * Initializes frame allocator.
 *
 * Called once at system boot.
 */
void frame_init(void) {
}

/**
 * Allocate continuous sequence of physical frames.
 *
 * The allocated frames can be returned by frame_free.
 *
 * Note that frame_free must be given the number of frames being freed.
 * This means that the frame allocator does not need to record the number
 * of frames allocated (only their allocation state: allocated or free).
 *
 * The provided tests will always call this function with count being a power
 * of two (and at least 1). But the implementation may choose to not restrict
 * the caller with this requirement. The restriction is meant to allow
 * implementing a class of frame allocators which require it.
 *
 * @param count How many frames to allocate.
 * @param phys Output-value pointer to the physical address of the
 * first frame in sequence.
 * @return Error code.
 * @retval EOK Frames allocated.
 * @retval ENOMEM Not enough memory.
 */
errno_t frame_alloc(size_t count, uintptr_t* phys) {
    assert(count > 0);

    // Feel free to remove this assertion if your allocator does not require
    // it (see the reasoning in the documentation comment above).
    assert(is_power_of_two(count));

    return ENOMEM;
}

/**
 * Free continuous sequence of physical frames.
 *
 * The returned frames were previously allocated by frame_alloc.
 *
 * @param count How many frames to free.
 * @param phys Physical address of the first frame in sequence.
 * @return Error code.
 * @retval EOK Frames freed.
 * @retval ENOENT Invalid frame address.
 * @retval EBUSY Some frames were not allocated (double free).
 */
errno_t frame_free(size_t count, uintptr_t phys) {
    assert(count > 0);

    // Feel free to remove this assertion if your allocator does not require
    // it (see the reasoning in the documentation comment of frame_alloc).
    assert(is_power_of_two(count));

    return ENOIMPL;
}
