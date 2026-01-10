// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Userspace binary bootstrap code.
 * This code will be placed at a fixed (virtual) address 0x4000.
 *
 * We place it in a special section so we can reference it from a linker
 * script.
 */

/* ABI requirement for stack alignment */
#define ABI_STACK_ALIGNMENT 16

.section .entry, "ax"

.global __entry

__entry:
    andi sp, sp, ~(ABI_STACK_ALIGNMENT - 1)
    jal __main
    /* Unreachable. */
    ebreak
.end __entry
