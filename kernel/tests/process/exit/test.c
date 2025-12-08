// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Check that process_join reports actual exit code.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <proc/process.h>

void kernel_test(void) {
    ktest_start("process/exit");

    process_t* app;
    errno_t err = process_create(&app, PROCESS_IMAGE_START, PROCESS_IMAGE_SIZE, PROCESS_MEMORY_SIZE);
    ktest_assert_errno(err, "process_create");

    int exit_status;
    err = process_join(app, &exit_status);
    ktest_assert_errno(err, "process_join");
    ktest_assert(exit_status == 5, "Unexpected exit status (got %d but wanted 5)", exit_status);

    ktest_passed();
}
