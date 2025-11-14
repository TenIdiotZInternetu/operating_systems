// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/bitmap.h>
#include <ktest.h>
#include <ktest/frame.h>
#include <mm/frame.h>

/*
 * Checks that bitmap is dynamically allocated.
 */

/* Expected bitmap size for all frames. */
#define BITMAP_SIZE_FOR_ALL_FRAMES BITMAP_GET_STORAGE_SIZE(FRAME_COUNT_MAX / 2)

// End of kernel code
extern uint8_t _kernel_code_end[0];

// End of kernel segment (including all "global" variables)
extern uint8_t _kernel_end[0];

void kernel_test(void) {
    ktest_start("frame/dynamic");

    uintptr_t kernel_vars_start = (uintptr_t)_kernel_code_end;
    uintptr_t kernel_vars_end = (uintptr_t)_kernel_end;

    size_t kernel_vars_size = kernel_vars_end - kernel_vars_start;

    printk("Kernel global (static) variables occupy %u bytes (about %uKB).\n",
            kernel_vars_size, kernel_vars_size / 1024);

    ktest_assert(kernel_vars_size < BITMAP_SIZE_FOR_ALL_FRAMES, "kernel variables contain frame bitmap");

    ktest_passed();
}
