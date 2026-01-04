// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>
#include <stdio.h>

/*
 * Checks that %d works in printf.
 */

int main(void) {
    utest_start("printf/int");

    puts(UTEST_EXPECTED "This is forty two: 42.");
    printf(UTEST_ACTUAL "This is forty two: %d.\n", 42);

    puts(UTEST_EXPECTED "This is minus five: -5.");
    printf(UTEST_ACTUAL "This is minus five: %d.\n", -5);

    puts(UTEST_EXPECTED "-2 -1 0 1 2.");
    printf(UTEST_ACTUAL "%d %d %d %d %d.\n", -2, -1, 0, 1, 2);

    puts(UTEST_EXPECTED "INT_MIN = -2147483648 ; INT_MAX 2147483647.");
    printf(UTEST_ACTUAL "INT_MIN = %d ; INT_MAX %d.\n", 0x80000000, 0x7fffffff);

    utest_passed();

    return 0;
}
