// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_NP_TYPES_H
#define _LIBC_NP_TYPES_H

#include <stdint.h>

/*
 * Various useful types.
 */

typedef signed long long int64_t;
typedef unsigned long long uint64_t;

typedef int32_t native_t;
typedef uint32_t unative_t;
typedef uint32_t uintptr_t;
typedef uint32_t off_t;

#define _CHECK_TYPE_SIZE(type, expected_size) \
    _Static_assert(sizeof(type) == expected_size, #type " should be " #expected_size "B")

_CHECK_TYPE_SIZE(int64_t, 8);
_CHECK_TYPE_SIZE(uint64_t, 8);

#endif
