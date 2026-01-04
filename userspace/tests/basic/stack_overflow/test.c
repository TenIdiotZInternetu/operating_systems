// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>
#include <stddef.h>
#include <stdio.h>

/*
 * Checks that infinite recursion eventually exhausts the stack, causing
 * page fault and forceful process termination.
 *
 * We disable GCC's infinite recursion checks to compile the code.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"

static void recurse(char* arg) {
    char space_eater[2048];
    if (arg != NULL) {
        arg[0] = 0x55;
    }
    recurse(space_eater);
}

#pragma GCC diagnostic pop

int main(void) {
    utest_start("basic/stackoverflow");

    utest_expect_abort();

    puts("Starting endless recursion...");

    recurse(NULL);

    puts("Survived endless recursion.");
    utest_failed();

    return 0;
}
