// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/bitmap.h>
#include <adt/bits.h>
#include <debug.h>
#include <exc.h>
#include <mm/frame.h>
#include <mm/heap.h>
#include <mm/self.h>
#include <types.h>

#define FRAME_ALLOCATOR_USED

#define FRAME_BITS_SIZE BITMAP_GET_STORAGE_SIZE(FRAME_COUNT_MAX)

static bitmap_t frame_bits;
static frame_allocator_t frame_allocator;

static uintptr_t first_frame_addr;

static uintptr_t _align(uintptr_t addr) {
    if (addr % FRAME_SIZE == 0) {
        return addr;
    } else {
        uintptr_t frames_before = (addr / FRAME_SIZE);
        return FRAME_SIZE * (frames_before + 1);
    }
}

static bool _has_free_frames(size_t n_frames) {
    return n_frames <= frame_allocator.free_frame_count;
}

static size_t _get_current_usable_frame_count_from(uintptr_t start_address) {
    return (start_address - first_frame_addr) / FRAME_SIZE;
}

/**
 * Updates the frame allocator's `free_frame_count` counter.
 *
 * The updated value is capped at `FRAME_COUNT_MAX`.
 */
void update_free_frame_count(uintptr_t start_address) {
    size_t free_frames_from_heap_start = _get_current_usable_frame_count_from(start_address);

    size_t max_frame_count = frame_bits.length;
    frame_allocator.free_frame_count = (free_frames_from_heap_start < max_frame_count)
            ? free_frames_from_heap_start
            : max_frame_count;
}

uintptr_t get_first_frame_addr(void) {
    return first_frame_addr;
}

size_t get_frame_region_size(void) {
    size_t available_size = (size_t)(get_memory_end() - first_frame_addr);
    return available_size / FRAME_REGION_MEMORY_DIVISOR;
}

/**
 * Initializes frame allocator.
 *
 * Called once at system boot.
 */
void frame_init(void) {
    first_frame_addr = _align(kernel_get_image_end());
    size_t frame_region_size = get_frame_region_size();
    size_t frame_count = frame_region_size / FRAME_SIZE;

    bitmap_init(&frame_bits, frame_count, (uint8_t*)first_frame_addr);
    size_t bitmap_size_in_frames = _align(frame_bits.length) / FRAME_SIZE;
    assert(bitmap_size_in_frames > 0);
    bitmap_fill_range(&frame_bits, 0, bitmap_size_in_frames);
    frame_allocator.free_frame_count -= bitmap_size_in_frames;
}

static errno_t _unsafe_frame_alloc(size_t count, uintptr_t* phys) {
    assert(count > 0);
    size_t frame_idx;
    errno_t err = bitmap_find_range(&frame_bits, count, false, &frame_idx);

    if (!_has_free_frames(count) || err == ENOENT) {
        return ENOMEM;
    }

    *phys = first_frame_addr + frame_idx * FRAME_SIZE;
    bitmap_fill_range(&frame_bits, frame_idx, count);
    frame_allocator.free_frame_count -= count;
    return EOK;
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
    bool interrupts_enabled = interrupts_disable();
    errno_t err = _unsafe_frame_alloc(count, phys);
    interrupts_restore(interrupts_enabled);
    return err;
}

static errno_t _unsafe_frame_free(size_t count, uintptr_t phys) {
    assert(count > 0);

    if (phys < first_frame_addr || phys % FRAME_SIZE != 0) {
        return ENOENT;
    }

    size_t frame_idx = (phys - first_frame_addr) / FRAME_SIZE;

    if (!bitmap_check_range_is(&frame_bits, frame_idx, count, true)) {
        return EBUSY;
    }

    bitmap_clear_range(&frame_bits, frame_idx, count);
    frame_allocator.free_frame_count += count;
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
    bool interrupts_enabled = interrupts_disable();
    errno_t err = _unsafe_frame_free(count, phys);
    interrupts_restore(interrupts_enabled);
    return err;
}

bool frame_has_enough_space(size_t bytes) {
    return frame_allocator.free_frame_count > bytes / FRAME_SIZE;
}

void print_frames() {
    bool enabled = interrupts_disable();
    size_t max_frames = get_frame_region_size() / FRAME_SIZE;
    size_t free_frames = frame_allocator.free_frame_count;

    printk("{ FRAME ALLOCATOR\n");
    printk("    used: %u/%u,\n", max_frames - free_frames, max_frames);
    printk("    free: %u,\n", free_frames);
    printk("    range: %p-%p\n", first_frame_addr, first_frame_addr + get_frame_region_size());
    printk("\n  USED FRAMES:\n");
    print_bitmap(&frame_bits);
    printk("\n}\n");

    interrupts_restore(enabled);
}
