// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Charles University

#include <np/utest.h>
#include <stdio.h>

int main(void) {
    char* beef = (char*)0xbeef;
    char* deadbeef = (char*)0xdeadbeef;

    puts(UTEST_EXPECTED "beef pointer: 0xbeef.");
    printf(UTEST_ACTUAL "beef pointer: %p.\n", beef);

    puts(UTEST_EXPECTED "dead beef pointer: 0xdeadbeef.");
    printf(UTEST_ACTUAL "dead beef pointer: %p.\n", deadbeef);

    puts(UTEST_EXPECTED "This is NULL pointer: (nil).");
    printf(UTEST_ACTUAL "This is NULL pointer: %p.\n", NULL);

    utest_passed();

    return 0;
}
