// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>
#include <stdio.h>

/*
 * Checks that %u works in printf.
 */

int main(void) {
    utest_start("printf/uint");

    puts(UTEST_EXPECTED "This is forty two: 42.");
    printf(UTEST_ACTUAL "This is forty two: %u.\n", 42);

    puts(UTEST_EXPECTED "0 1 2.");
    printf(UTEST_ACTUAL "%u %u %u.\n", 0, 1, 2);

    puts(UTEST_EXPECTED "MAX_UINT is 4294967295.");
    printf(UTEST_ACTUAL "MAX_UINT is %u.\n", (unsigned int)0xffffffff);

    utest_passed();

    return 0;
}
