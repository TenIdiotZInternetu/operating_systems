// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <debug/code.h>
#include <drivers/csr.h>
#include <drivers/machine.h>
#include <drivers/timer.h>
#include <exc.h>
#include <proc/scheduler.h>

/** Handles general exception.
 *
 * The function receives a pointer to an exception context, which
 * represents a snapshot of CPU and (some) CSRs at the
 * time of the exception occurring.
 */
void handle_exception_general(exc_context_t* exc_context) {
    if (csr_scause_is_interrupt_pending(exc_context->scause)) {
        scheduler_schedule_next();
    } else {
        machine_enter_debugger();
    }
}
