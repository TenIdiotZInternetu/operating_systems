// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>
#include <stdio.h>

/*
 * Checks that %s works in printf.
 */

int main(void) {
    utest_start("printf/string");

    puts(UTEST_EXPECTED "This is ONE string.");
    printf(UTEST_ACTUAL "This is %s string.\n", "ONE");

    // Check that newline is not appended automatically.
    puts(UTEST_EXPECTED "This is another string.");
    printf(UTEST_ACTUAL "This is");
    printf(" another ");
    printf("string.\n");

    puts(UTEST_EXPECTED "This is FIRST and SECOND string.");
    printf(UTEST_ACTUAL "This is %s and %s string.\n", "FIRST", "SECOND");

    // This is actually undefined behaviour for standard printf
    // but it is probably safer to test for it explicitly...
    puts(UTEST_EXPECTED "This is NULL string: (null).");
    printf(UTEST_ACTUAL "This is NULL string: %s.\n", NULL);

    utest_passed();

    return 0;
}
