// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/proc.h>
#include <np/utest.h>
#include <stddef.h>

/*
 * Checks that passing NULL to proc_info_get does not terminate the program.
 */

int main(void) {
    utest_start("procinfo/bad1");

    bool ok = np_proc_info_get(NULL);

    utest_assert(!ok, "NULL shall not abort the program");

    utest_passed();

    return 0;
}
