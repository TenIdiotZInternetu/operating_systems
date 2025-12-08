// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/machine.h>
#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>

/** Handles a syscall.
 *
 * The function receives a pointer to an exception context, which
 * represents the snapshot of CPU and (some) CP0 registers at the
 * time of the syscall.
 */
void handle_syscall(exc_context_t* exc_context) {
    machine_enter_debugger();

    // On success, shift EPC by 4 to resume execution of the interrupted
    // thread on the next instruction (we don't want to restart it).
    exc_context->sepc += 4;
}
