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
unative_t get_next_satp(void) {
    // See Figure 4.14 of https://d3s.mff.cuni.cz/files/teaching/nswi200/202324/doc/riscv-spec-privileged.pdf
    // For now ignore ASID, MODE depends on whether the thread belongs to an address space
    // PPN is the root table's address >> 12

    thread_t* p_thread_to_run = scheduler_get_thread_to_run();

    if (p_thread_to_run->address_space == NULL) {
        return 0;
    }

    unative_t mode = 1;
    unative_t ppn = p_thread_to_run->address_space->root_table_addr >> 12;

    return (mode << 31) | ppn;
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
    } else if (exc_context->scause == EXC_CODE_ECALL_U) {
        handle_syscall(exc_context);
    } else if (exc_context->scause == EXC_CODE_ILLEGAL_INSTRUCTION) {
        thread_t* cur_thread = scheduler_get_current_thread();
        thread_kill(cur_thread);
    } else if (csr_scause_is_page_fault(exc_context->scause)) {
        as_handle_page_fault(exc_context->stval);
    } else {
        printk("Uncaught exception: %u\n", exc_context->scause);
        machine_enter_debugger();
    }
}
