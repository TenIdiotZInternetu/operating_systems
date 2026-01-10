// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Charles University

#include <np/utest.h>
#include <stdio.h>

int main(void) {
    puts(UTEST_EXPECTED "This is string with %s.");
    printf(UTEST_ACTUAL "This is string with %s.\n", "%s");

    puts(UTEST_EXPECTED "This is string with %%.");
    printf(UTEST_ACTUAL "%s.\n", "This is string with %%");

    utest_passed();

    return 0;
}
