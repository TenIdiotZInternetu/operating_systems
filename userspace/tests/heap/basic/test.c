// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "../theap.h"
#include <np/utest.h>
#include <stdlib.h>

/*
 * Tests that malloc() returns a valid address. We assume
 * there would be always enough free memory to allocate 8 bytes
 * when the application starts.
 */

int main(void) {
    utest_start("heap/basic");

    void* ptr = malloc(8);
    utest_assert(ptr != NULL, "no memory available");
    utest_check_malloc_result(ptr, 8);
    utest_check_malloc_writable(ptr);

    utest_passed();

    return 0;
}
