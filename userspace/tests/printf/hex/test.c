// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>
#include <stdio.h>

/*
 * Checks that %x and %X works in printf.
 */

int main(void) {
    utest_start("printf/hex");

    puts(UTEST_EXPECTED "Zero is still zero: 0.");
    printf(UTEST_ACTUAL "Zero is still zero: %x.\n", 0);

    puts(UTEST_EXPECTED "This is forty two: 2a.");
    printf(UTEST_ACTUAL "This is forty two: %x.\n", 42);

    puts(UTEST_EXPECTED "This is minus five: fffffffb.");
    printf(UTEST_ACTUAL "This is minus five: %x.\n", -5);

    puts(UTEST_EXPECTED "INT_MIN in hexa: 80000000 ; INT_MAX in hexa: 7fffffff.");
    printf(UTEST_ACTUAL "INT_MIN in hexa: %x ; INT_MAX in hexa: %x.\n", 0x80000000, 0x7fffffff);

    puts(UTEST_EXPECTED "UINT_MAX in hexa: ffffffff.");
    printf(UTEST_ACTUAL "UINT_MAX in hexa: %x.\n", 0xffffffff);

    puts(UTEST_EXPECTED "Capital letters: 0xABCD.");
    printf(UTEST_ACTUAL "Capital letters: 0x%X.\n", 0xabcd);

    utest_passed();

    return 0;
}
