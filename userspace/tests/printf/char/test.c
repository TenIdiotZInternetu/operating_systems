// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>
#include <stdio.h>

/*
 * Checks that %c works in printf.
 */

int main(void) {
    utest_start("printf/char");

    puts(UTEST_EXPECTED "Hello, World.");
    printf(UTEST_ACTUAL "%c%c%c%c%c, World.\n", 'H', 'e', 'l', 'l', 'o');

    puts(UTEST_EXPECTED "Test percent: %c with '%' prints '%'.");
    printf(UTEST_ACTUAL "Test percent: %%c with '%%' prints '%c'.\n", '%');

    utest_passed();

    return 0;
}
