// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/machine.h>
#include <drivers/printer.h>
#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>
#include <proc/thread.h>

static void _handle_exit(int code) {
    thread_t* cur_thread = thread_get_current();
    assert(cur_thread != NULL);

    cur_thread->process->exit_status = code;
    thread_finish(NULL);
}

static void _handle_putchar(unative_t char_to_print) {
    printer_putchar((char)char_to_print);
}

static void _handle_write(unative_t buffer_address, unative_t count) {
    if (buffer_address == 0 || count == 0) {
        return;
    }

    const char * char_buffer = (const char *)buffer_address;

    for (unative_t i = 0; i < count; ++i) {
        printer_putchar(char_buffer[i]);
    }
}

/** Handles a syscall.
 *
 * The function receives a pointer to an exception context, which
 * represents the snapshot of CPU and (some) CP0 registers at the
 * time of the syscall.
 */
void handle_syscall(exc_context_t* exc_context) {
    syscall_t syscall_id = exc_context->a0;

    switch (syscall_id) {
        case SYSCALL_EXIT:
            _handle_exit(exc_context->a1);
            break;
        case SYSCALL_PUTCHAR:
            _handle_putchar(exc_context->a1);
            break;
        case SYSCALL_WRITE:
            _handle_write(exc_context->a1, exc_context->a2);
            break;
        default:
            exc_context->a0 = ENOIMPL;
            break;
    }

    // On success, shift EPC by 4 to resume execution of the interrupted
    // thread on the next instruction (we don't want to restart it).
    exc_context->sepc += 4;
}
