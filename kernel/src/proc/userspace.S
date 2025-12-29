// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

.text

#include <drivers/csr.h>

/*
 * void cpu_jump_to_userspace(
 *     uintptr_t a0_stack_top,
 *     uintptr_t a1_entry_func
 * )
 *
 * Switches processor to userspace code. The first argument is address
 * of the top of userland stack, the second argument is the address where
 * to continue the execution (i.e. userspace entry point).
 *
 * The jump is done via setting status register to emulate exception handling,
 * setting EPC and stack pointer and then executing the SRET that leaves the
 * exception level state and "returns" to userspace.
 */
.globl cpu_jump_to_userspace

cpu_jump_to_userspace:
    /*
     * Emulate exception handling. We clear the SPP bit to pretend the
     * came from userland and disable interrupts so that we can safely
     * set EPC to entry function and set new stack.
     *
     * When running in user mode, the interrupts are automatically enabled.
     */
    li t0, CSR_SSTATUS_SPP_MASK | CSR_SSTATUS_SIE
    csrc sstatus, t0

    csrw sepc, a1
    move sp, a0

    sret
.end cpu_jump_to_userspace
