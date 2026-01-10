// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "../theap.h"
#include <np/utest.h>
#include <stdlib.h>

/*
 * Tests that free() does not fail with NULL.
 */

int main(void) {
    utest_start("heap/null");

    free(NULL);

    utest_passed();

    return 0;
}
