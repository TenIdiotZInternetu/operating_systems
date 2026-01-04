// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <abi.h>
#include <drivers/csr.h>
#include <exc/context.h>

.data

/*
 * Pointer to the top of the kernel stack of the current thread.
 *
 * The value is only read by user-space threads entering the kernel to
 * service an exception, because they need to switch from a user space
 * stack to their (original) kernel stack.
 *
 * However, the value is written by all threads in two places. The first
 * is the cpu_switch_context function, which stores the stack top of the
 * next thread here. This does not affect kernel-only threads. The other
 * place is near the end of the exception handler, where we need to make
 * sure that the value for the current thread remains valid.
 *
 * This is not very nice, but we don't need to know the layout of the
 * thread_t structure and we can do it transparently. Alternatively, the
 * stack top would have to be supplied explicitly before each voluntary
 * context switch.
 */
.align 4
.globl kernel_stack_top
kernel_stack_top:
    .word 0


.section .handlers

.balign 4
.global supervisor_trap_handler
supervisor_trap_handler:
    // Store s0 to sscratch
    csrw sscratch, s0

    // Check if we are coming from userspace or supervisor
    csrr s0, sstatus
    andi s0, s0, CSR_SSTATUS_SPP_MASK
    bnez s0, from_supervisor

from_userland:
    // We need to switch to kernel stack because we are coming from userspace
    lw s0, kernel_stack_top
    addi s0, s0, -EXC_CONTEXT_SIZE
    j common_handling

from_supervisor:
    // * Move the stack of the trapped thread down if we are coming from S-mode */
    addi s0, sp, -EXC_CONTEXT_SIZE

common_handling:
    // Align stack as is required by the ABI
    andi s0, s0, ~(ABI_STACK_ALIGNMENT - 1)
    // Save context to stack
    EXC_CONTEXT_SAVE_GENERAL_REGISTERS s0

    // Move the address of the stored registers to a0
    mv sp, s0
    // restore s0
    csrr s0, sscratch

    // Read CSRs
    csrr t0, sstatus
    csrr t1, scause
    csrr t2, sepc
    csrr t3, stval

    // Store the read values to stack
    sw t0, EXC_CONTEXT_SSTATUS_OFFSET(sp)
    sw t1, EXC_CONTEXT_SCAUSE_OFFSET(sp)
    sw t2, EXC_CONTEXT_SEPC_OFFSET(sp)
    sw t3, EXC_CONTEXT_STVAL_OFFSET(sp)

    // The stack pointer is the address of the context structure
    mv a0, sp

    // Call the C function
    jal handle_exception_general

    // Restore context

    // First restore kernel_stack_top to its original value
    mv t0, sp
    addi t0, t0, EXC_CONTEXT_SIZE
    addi t0, t0, ABI_STACK_ALIGNMENT - 1
    andi t0, t0, ~(ABI_STACK_ALIGNMENT - 1)
    la t1, kernel_stack_top
    sw t0, 0(t1)

    // First restore the CSRs
    lw t0, EXC_CONTEXT_SSTATUS_OFFSET(sp)
    lw t1, EXC_CONTEXT_SCAUSE_OFFSET(sp)
    lw t2, EXC_CONTEXT_SEPC_OFFSET(sp)
    lw t3, EXC_CONTEXT_STVAL_OFFSET(sp)

    csrw sstatus, t0
    csrw scause, t1
    csrw sepc, t2
    csrw stval, t3

    // Restore general purpose registers
    csrw sscratch, s0
    mv s0, sp
    EXC_CONTEXT_LOAD_GENERAL_REGISTERS s0
    csrr s0, sscratch

    sret
