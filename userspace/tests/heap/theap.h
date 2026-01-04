// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _TESTS_HEAP_THEAP_H
#define _TESTS_HEAP_THEAP_H

#include <np/types.h>

#define utest_check_malloc_result(addr_ptr, size) \
    do { \
        uintptr_t __addr = (uintptr_t)addr_ptr; \
        unsigned long long __addr_end = __addr + (size); \
        utest_assert_fmt(__addr < 0x80000000, \
                #addr_ptr " (0x%x) not in [0x00000000, 0x80000000)", __addr); \
        utest_assert_fmt(__addr_end < 0x80000000, \
                #addr_ptr " + " #size " (0x%x + %dB) not in [0x00000000, 0x80000000)", __addr, (size)); \
        utest_assert_fmt((__addr % 4) == 0, "bad alignment on 0x%x", __addr); \
    } while (0)

#define utest_check_malloc_writable(addr_ptr) \
    do { \
        volatile uint8_t* __ptr = (uint8_t*)addr_ptr; \
        __ptr[0] = 0xAA; \
        __ptr[1] = 0x55; \
        utest_assert_fmt(__ptr[0] == 0xAA, "expected 0xAA, got 0x%x", __ptr[0]); \
        utest_assert_fmt(__ptr[1] == 0x55, "expected 0x55, got 0x%x", __ptr[1]); \
    } while (0)

#endif
