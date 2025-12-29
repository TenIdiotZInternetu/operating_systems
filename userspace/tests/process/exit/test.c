// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>

/**
 * Check that non-zero return code is correctly propagated.
 */

int main(void) {
    utest_start("process/exit");

    puts("Terminating with exit code 5.");

    return 5;
}
