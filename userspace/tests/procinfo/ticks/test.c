// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/proc.h>
#include <np/utest.h>

/*
 * Checks that reading ticks from proc_info_get is monotonous.
 */

static volatile int value;

static void work(void) {
    for (int i = 0; i < 10000; i++) {
        value = i;
    }
}

int main(void) {
    utest_start("procinfo/ticks");

    np_proc_info_t info1, info2, info3;
    bool ok1, ok2, ok3;

    ok1 = np_proc_info_get(&info1);
    work();
    ok2 = np_proc_info_get(&info2);
    work();
    ok3 = np_proc_info_get(&info3);

    utest_assert(ok1, "first call failed");
    utest_assert(ok2, "second call failed");
    utest_assert(ok3, "third call failed");

    utest_assert(info1.total_ticks < info2.total_ticks, "ticks must grow");
    utest_assert(info2.total_ticks < info3.total_ticks, "ticks must grow");

    utest_passed();

    return 0;
}
