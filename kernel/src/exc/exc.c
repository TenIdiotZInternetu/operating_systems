// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <debug/code.h>
#include <drivers/csr.h>
#include <drivers/machine.h>
#include <drivers/sv32.h>
#include <drivers/timer.h>
#include <exc.h>
#include <proc/scheduler.h>

/**
 * Assembles the satp of the next scheduled thread's address space
 */
static unative_t get_next_satp() {
    // TODO
    // See Figure 4.14 of https://d3s.mff.cuni.cz/files/teaching/nswi200/202324/doc/riscv-spec-privileged.pdf
    // For now ignore ASID, MODE depends on whether the thread belongs to an address space
    // PPN is the root table's address >> 12

    // Maybe there's a more appropriate file for this function
    return 0;
}

/** Handles general exception.
 *
 * The function receives a pointer to an exception context, which
 * represents a snapshot of CPU and (some) CSRs at the
 * time of the exception occurring.
 */
void handle_exception_general(exc_context_t* exc_context) {
    if (csr_scause_is_interrupt_pending(exc_context->scause)) {
        sv32_set_satp_tlb_shootdown(get_next_satp());
        scheduler_schedule_next();
    } else if (csr_scause_is_page_fault(exc_context->scause)) {
        // TODO
        // Allocate new frame for the virtual address in context->stval
        // or kill the thread if it accesses NULL pages or kernel pages
    } else {
        machine_enter_debugger();
    }
}
