// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/syscall.h>
#include <stdlib.h>

/** Terminate current process.
 *
 * @param status Process exit code.
 */
void exit(int status) {
    __SYSCALL1(SYSCALL_EXIT, (unative_t)status);
}
