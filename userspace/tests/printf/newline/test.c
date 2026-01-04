// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/utest.h>
#include <stdio.h>

int main(void) {
    puts(UTEST_BLOCK_EXPECTED "This is first line.");
    puts(UTEST_BLOCK_EXPECTED "This is second line.");
    puts(UTEST_BLOCK_EXPECTED "This is last line.");

    printf("This is %s", "xxfirst" + 2);
    printf(" line.");
    printf("\n");

    printf("This is second line.");
    printf("\n");

    printf("This is %s %s", "last", "line");
    puts(".");

    puts(UTEST_BLOCK_EXPECTED "This is first line.");
    puts(UTEST_BLOCK_EXPECTED "This is 2nd line.");
    printf("This is %s line.\nThis is %dnd line.\n", "first", 2);

    puts(UTEST_BLOCK_EXPECTED "This is another block of lines.");
    puts(UTEST_BLOCK_EXPECTED "This is second line.");
    puts(UTEST_BLOCK_EXPECTED "This is third line.");
    printf("This is another block of lines.%s", "\nThis is second line.\nThis is third line.\n");

    puts(UTEST_EXPECTED "Check that no percent sign forces new-line.");
    printf(UTEST_ACTUAL "Check that no percent sign forces %s.\n", "new-line");

    puts(UTEST_EXPECTED "Check that no percent sign forces new-line: 42.");
    printf(UTEST_ACTUAL "Check that no percent sign forces new-line: %d.\n", 42);

    puts(UTEST_EXPECTED "Check that no percent sign forces new-line: 42.");
    printf(UTEST_ACTUAL "Check that no percent sign forces new-line: %u.\n", 42);

    puts(UTEST_EXPECTED "Check that no percent sign forces new-line: 2a.");
    printf(UTEST_ACTUAL "Check that no percent sign forces new-line: %x.\n", 42);

    puts(UTEST_EXPECTED "Check that no percent sign forces new-line: 2A.");
    printf(UTEST_ACTUAL "Check that no percent sign forces new-line: %X.\n", 42);

    puts(UTEST_EXPECTED "Check that no percent sign forces new-line: !.");
    printf(UTEST_ACTUAL "Check that no percent sign forces new-line: %c.\n", '!');

    puts(UTEST_EXPECTED "Check that no percent sign forces new-line: (nil).");
    printf(UTEST_ACTUAL "Check that no percent sign forces new-line: %p.\n", NULL);

    utest_passed();

    return 0;
}
