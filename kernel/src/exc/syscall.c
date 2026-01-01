// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/machine.h>
#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>
#include <proc/thread.h>

static void _handle_exit(syscall_t code) {
    thread_t* cur_thread = thread_get_current();
    assert(cur_thread != NULL);

    cur_thread->process->exit_status = code;
    thread_finish(NULL);
}

/** Handles a syscall.
 *
 * The function receives a pointer to an exception context, which
 * represents the snapshot of CPU and (some) CP0 registers at the
 * time of the syscall.
 */
void handle_syscall(exc_context_t* exc_context) {
    syscall_t syscall_id = exc_context->a0;

    if (syscall_id == SYSCALL_EXIT) {
        _handle_exit(exc_context->a1);
    }

    // On success, shift EPC by 4 to resume execution of the interrupted
    // thread on the next instruction (we don't want to restart it).
    exc_context->sepc += 4;
}
