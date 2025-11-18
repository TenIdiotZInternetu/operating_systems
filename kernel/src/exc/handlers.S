// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <abi.h>
#include <drivers/csr.h>
#include <exc/context.h>


.section .handlers

.balign 4
.global supervisor_trap_handler
supervisor_trap_handler:
    // Store s0 to sscratch
    csrw sscratch, s0

    // Move the stack of the trapped thread down
    addi s0, sp, -EXC_CONTEXT_SIZE
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
