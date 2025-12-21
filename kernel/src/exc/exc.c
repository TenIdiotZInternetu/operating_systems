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
    // See Figure 4.14 of https://d3s.mff.cuni.cz/files/teaching/nswi200/202324/doc/riscv-spec-privileged.pdf
    // For now ignore ASID, MODE depends on whether the thread belongs to an address space
    // PPN is the root table's address >> 12

    thread_t *p_thread_to_run = scheduler_get_thread_to_run();

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
    } else if (csr_scause_is_page_fault(exc_context->scause)) {
        // Allocate new frame for the virtual address in context->stval
        // or kill the thread if it accesses NULL pages or kernel pages

        uintptr_t page_fault_address = exc_context->stval;

        thread_t *p_curr_thread = scheduler_get_current_thread();
        as_t *p_curr_thread_as = p_curr_thread->address_space;

        if (is_illegal_memory_access(page_fault_address, p_curr_thread_as)) {
            thread_kill(p_curr_thread);
            return;
        }

        uintptr_t aligned_page_fault_address = page_fault_address & PAGE_SIZE_ALIGNMENT_MASK;

        uintptr_t alloc_frame_phys_address = as_alloc_page(p_curr_thread_as, aligned_page_fault_address, 1);

        if (alloc_frame_phys_address == 0) {
            thread_kill(p_curr_thread);
        }
    } else {
        machine_enter_debugger();
    }
}
