// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/types.h>
#include <np/utest.h>
#include <stddef.h>
#include <stdio.h>

/*
 * Checks that NULL pointer dereference causes forced process termination.
 */

static volatile uintptr_t almost_null_address = 32;

int main(void) {
    utest_start("basic/null_deref");
    utest_expect_abort();

    uint8_t* cause_page_fault = (uint8_t*)almost_null_address;
    *cause_page_fault = 42;

    puts("Survived access to NULL.");

    utest_failed();

    return 0;
}
