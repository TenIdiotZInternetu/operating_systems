// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_ASSERT_H
#define _LIBC_ASSERT_H

#include <stdio.h>
#include <stdlib.h>

#define __QUOTE_ME_(x) #x
#define __QUOTE_ME(x) __QUOTE_ME_(x)

#define assert(expr) \
    do { \
        if (!(expr)) { \
            puts("Assertion failed at " __FILE__ ":" __QUOTE_ME(__LINE__) ": " #expr); \
            exit(-1); \
        } \
    } while (0)

#endif
