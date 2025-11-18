// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/bitmap.h>
#include <adt/bits.h>
#include <debug.h>
#include <mm/frame.h>
#include <mm/self.h>
#include <types.h>

#define FRAME_BITS_SIZE BITMAP_GET_STORAGE_SIZE(FRAME_COUNT_MAX)

static bitmap_t frame_bits;
static bitmap_unit_t frame_bits_data[FRAME_BITS_SIZE];

uintptr_t memory_start;

static uintptr_t _align(uintptr_t addr) {
    if (addr % FRAME_SIZE == 0) {
        return addr;
    } else {
        uintptr_t frames_before = (addr / FRAME_SIZE);
        return FRAME_SIZE * (frames_before + 1);
    }
}

/**
 * Initializes frame allocator.
 *
 * Called once at system boot.
 */
void frame_init(void) {
    bitmap_init(&frame_bits, FRAME_BITS_SIZE, frame_bits_data);
    memory_start = _align(kernel_get_image_end());
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
    size_t frame_idx;
    errno_t err = bitmap_find_range(&frame_bits, count, false, &frame_idx);

    if (err == ENOENT) {
        return ENOMEM;
    }

    *phys = memory_start + frame_idx * FRAME_SIZE;
    bitmap_fill_range(&frame_bits, frame_idx, count);
    return EOK;
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

    // TODO: Check if phys is a multiple of FRAME_SIZE and that its within allowed range

    size_t frame_idx = (phys - memory_start) / FRAME_SIZE;

    if (!bitmap_check_range_is(&frame_bits, frame_idx, count, true)) {
        return EBUSY;
    }

    bitmap_clear_range(&frame_bits, frame_idx, count);
    return EOK;
}

uintptr_t frames_end_address() {
    return memory_start + FRAME_SIZE * FRAME_COUNT_MAX;
}
