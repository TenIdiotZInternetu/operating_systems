// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _EXC_CONTEXT_H
#define _EXC_CONTEXT_H

/*
 * This file is shared between C and assembler sources, because we
 * want to keep information on low-level structures in the same file.
 * We use ifdefs to include the portions relevant to each context.
 */

/** Size of the exc_context_t structure below, needed in assembler code. */
#define EXC_CONTEXT_SIZE (21 * 4)

#ifndef __ASSEMBLER__

#include <types.h>

/**
 * CPU context for exception handling
 *
 * This context structure is used to store the state of a thread when its
 * execution is interrupted as a result of exception, external interrupt,
 * or a syscall. These events are different from voluntary context switch
 * where most of the context is already stored (implicitly) on the stack.
 * In this case, we need to store almost everything.
 *
 * We do not need to store the zero, gp, and tp registers.
 * Also, if we can avoid using the callee-saved registers (s0/fp and s1-s11)
 * before jumping into C code of the handler, they will be preserved
 * automatically by the compiler in any function that uses them.
 *
 * Finally, we need to store additional CSRs so that the handler
 * can identify the cause of the exception and (if possible) resume thread
 * execution after the exception has been handled.
 */
typedef struct {
    /* x0
     * No need to store zero.
     */

    /* x1 */
    unative_t ra;

    /* x2 */
    unative_t sp;

    /* x3
     * gp - No need to store
     */

    /* x4
     * tp - No need to store
     */

    /* x5..x7 */
    unative_t t0;
    unative_t t1;
    unative_t t2;

    /* x8..x9
     * s0..s1 - No need to store
     */

    /* x10..x17 */
    unative_t a0;
    unative_t a1;
    unative_t a2;
    unative_t a3;
    unative_t a4;
    unative_t a5;
    unative_t a6;
    unative_t a7;

    /* x18..x27
     * s2..s11 - No need to save
     */

    /* x28..x31 */
    unative_t t3;
    unative_t t4;
    unative_t t5;
    unative_t t6;

    /* CSRs */
    unative_t sstatus;
    unative_t scause;
    unative_t sepc;
    unative_t stval;
} exc_context_t;

#else /* __ASSEMBLER__ */
// clang-format off

/*
 * The offsets for general purpose and special registers
 * must match the exc_context_t structure.
 */
.set EXC_CONTEXT_RA_OFFSET, 0*4

.set EXC_CONTEXT_SP_OFFSET, 1*4

.set EXC_CONTEXT_T0_OFFSET, 2*4
.set EXC_CONTEXT_T1_OFFSET, 3*4
.set EXC_CONTEXT_T2_OFFSET, 4*4

.set EXC_CONTEXT_A0_OFFSET, 5*4
.set EXC_CONTEXT_A1_OFFSET, 6*4
.set EXC_CONTEXT_A2_OFFSET, 7*4
.set EXC_CONTEXT_A3_OFFSET, 8*4
.set EXC_CONTEXT_A4_OFFSET, 9*4
.set EXC_CONTEXT_A5_OFFSET, 10*4
.set EXC_CONTEXT_A6_OFFSET, 11*4
.set EXC_CONTEXT_A7_OFFSET, 12*4

.set EXC_CONTEXT_T3_OFFSET, 13*4
.set EXC_CONTEXT_T4_OFFSET, 14*4
.set EXC_CONTEXT_T5_OFFSET, 15*4
.set EXC_CONTEXT_T6_OFFSET, 16*4

.set EXC_CONTEXT_SSTATUS_OFFSET, 17*4
.set EXC_CONTEXT_SCAUSE_OFFSET, 18*4
.set EXC_CONTEXT_SEPC_OFFSET, 19*4
.set EXC_CONTEXT_STVAL_OFFSET, 20*4

/*
 * The EXC_CONTEXT_SAVE_GENERAL_REGISTERS macro stores general registers
 * into the ex_context_t structure. The macro can be used with any general
 * purpose register (no registers are destroyed in this code).
 */
.macro EXC_CONTEXT_SAVE_GENERAL_REGISTERS base

    sw ra, EXC_CONTEXT_RA_OFFSET(\base)

    sw sp, EXC_CONTEXT_SP_OFFSET(\base)

    sw t0, EXC_CONTEXT_T0_OFFSET(\base)
    sw t1, EXC_CONTEXT_T1_OFFSET(\base)
    sw t2, EXC_CONTEXT_T2_OFFSET(\base)

    sw a0, EXC_CONTEXT_A0_OFFSET(\base)
    sw a1, EXC_CONTEXT_A1_OFFSET(\base)
    sw a2, EXC_CONTEXT_A2_OFFSET(\base)
    sw a3, EXC_CONTEXT_A3_OFFSET(\base)
    sw a4, EXC_CONTEXT_A4_OFFSET(\base)
    sw a5, EXC_CONTEXT_A5_OFFSET(\base)
    sw a6, EXC_CONTEXT_A6_OFFSET(\base)
    sw a7, EXC_CONTEXT_A7_OFFSET(\base)

    sw t3, EXC_CONTEXT_T3_OFFSET(\base)
    sw t4, EXC_CONTEXT_T4_OFFSET(\base)
    sw t5, EXC_CONTEXT_T5_OFFSET(\base)
    sw t6, EXC_CONTEXT_T6_OFFSET(\base)

.endm EXC_CONTEXT_SAVE_GENERAL_REGISTERS

/*
 * The EXC_CONTEXT_LOAD_GENERAL_REGISTERS macro loads general registers from
 * the exc_context_t structure. The macro \base parameter should be a register
 * that is different from the registers being restored. The sp register is
 * an exception, because it is restored last (on purpose).
 *
 * Keep in mind that the macro restores the stack pointer, which means
 * that it will switch to another stack.
 */
.macro EXC_CONTEXT_LOAD_GENERAL_REGISTERS base

    lw ra, EXC_CONTEXT_RA_OFFSET(\base)

    lw t0, EXC_CONTEXT_T0_OFFSET(\base)
    lw t1, EXC_CONTEXT_T1_OFFSET(\base)
    lw t2, EXC_CONTEXT_T2_OFFSET(\base)

    lw a0, EXC_CONTEXT_A0_OFFSET(\base)
    lw a1, EXC_CONTEXT_A1_OFFSET(\base)
    lw a2, EXC_CONTEXT_A2_OFFSET(\base)
    lw a3, EXC_CONTEXT_A3_OFFSET(\base)
    lw a4, EXC_CONTEXT_A4_OFFSET(\base)
    lw a5, EXC_CONTEXT_A5_OFFSET(\base)
    lw a6, EXC_CONTEXT_A6_OFFSET(\base)
    lw a7, EXC_CONTEXT_A7_OFFSET(\base)

    lw t3, EXC_CONTEXT_T3_OFFSET(\base)
    lw t4, EXC_CONTEXT_T4_OFFSET(\base)
    lw t5, EXC_CONTEXT_T5_OFFSET(\base)
    lw t6, EXC_CONTEXT_T6_OFFSET(\base)

    lw sp, EXC_CONTEXT_SP_OFFSET(\base)

.endm EXC_CONTEXT_LOAD_GENERAL_REGISTERS

// clang-format on
#endif
#endif
