// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/types.h>
#include <np/utest.h>
#include <stddef.h>
#include <stdio.h>

/*
 * Checks that using control registers causes forced process termination.
 */

int main(void) {
    utest_start("basic/csr");
    utest_expect_abort();

    register unative_t reg_a7 __asm__("a7");
    __asm__ volatile("csrr a7, sstatus\n");

    puts("Survived access to control register.");
    printf("Read sstatus to have value %u.\n", reg_a7);

    utest_failed();

    return 0;
}
