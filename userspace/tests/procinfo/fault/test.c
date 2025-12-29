// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/proc.h>
#include <np/utest.h>
#include <stddef.h>

/*
 * Checks that the returned value of virt_mem_size is fine (i.e. we
 * can access memory up to this size and cause a page fault (SIGSEGV in Unix
 * world) on the next byte.
 */

volatile int* data;

int main(void) {
    utest_start("procinfo/fault");
    utest_expect_abort();

    np_proc_info_t info;
    bool ok = np_proc_info_get(&info);

    utest_assert(ok, "failed to retrieve process information");

    uintptr_t first_bad_address = info.virt_mem_size + NP_NULL_AREA_SIZE;
    data = ((int*)first_bad_address) - 1;

    *data = 42;
    utest_assert(*data == 42, "can write to the last bytes of process memory");

    data++;
    *data = 84;

    utest_failed();

    return 0;
}
