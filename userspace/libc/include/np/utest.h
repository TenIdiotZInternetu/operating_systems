// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_NP_UTEST_H
#define _LIBC_NP_UTEST_H

/*
 * Macros for userspace tests. Copy of kernel/ktest.h.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define UTEST_EXPECTED "[EXPECTED]: "
#define UTEST_BLOCK_EXPECTED "[EXPECTED BLOCK]: "
#define UTEST_ACTUAL "[ ACTUAL ]: "

#define utest_start(name) \
    puts("== USERSPACE TEST " name " ==")

#define utest_passed() \
    puts("\n\nTest finished.\n\n")

#define utest_failed() \
    do { \
        puts("\n\nTest failed.\n\n"); \
        exit(-1); \
    } while (0)

#define utest_expect_abort() \
    puts("\n\n[ ENDS WITH APPLICATION ABORT ]\n\n")

#define utest_assert(expr, message) \
    do { \
        if (!(expr)) { \
            puts("\n\n" __FILE__ ":" __QUOTE_ME(__LINE__) ": Userspace test assertion failed: " #expr); \
            puts(__FILE__ ":" __QUOTE_ME(__LINE__) ": " message); \
            utest_failed(); \
        } \
    } while (0)

#define utest_assert_fmt(expr, fmt, ...) \
    do { \
        if (!(expr)) { \
            puts("\n\n" __FILE__ ":" __QUOTE_ME(__LINE__) ": Userspace test assertion failed: " #expr); \
            printf(__FILE__ ":" __QUOTE_ME(__LINE__) ": " fmt "\n", ##__VA_ARGS__); \
            utest_failed(); \
        } \
    } while (0)

#endif
