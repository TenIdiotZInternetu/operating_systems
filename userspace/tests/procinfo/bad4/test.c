// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/proc.h>
#include <np/utest.h>
#include <stddef.h>

/*
 * Checks that using boundary address for the argument of the system call
 * is handled correctly.
 */

int main(void) {
    utest_start("procinfo/fault");

    np_proc_info_t info;
    bool ok = np_proc_info_get(&info);

    utest_assert(ok, "failed to retrieve process information");

    uintptr_t boundary_address = info.virt_mem_size + NP_NULL_AREA_SIZE - 4;
    ok = np_proc_info_get((np_proc_info_t*)boundary_address);
    utest_assert(!ok, "bad address shall not abort the program");

    utest_passed();

    return 0;
}
