// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_STDINT_H
#define _LIBC_STDINT_H

/*
 * Note that the type definitions heavy relies on our knowledge
 * of compiler configuration.
 */
typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed long int32_t;
typedef unsigned long uint32_t;

#define _CHECK_TYPE_SIZE(type, expected_size) \
    _Static_assert(sizeof(type) == expected_size, #type " should be " #expected_size "B")

_CHECK_TYPE_SIZE(int8_t, 1);
_CHECK_TYPE_SIZE(uint8_t, 1);
_CHECK_TYPE_SIZE(int16_t, 2);
_CHECK_TYPE_SIZE(uint16_t, 2);
_CHECK_TYPE_SIZE(int32_t, 4);
_CHECK_TYPE_SIZE(uint32_t, 4);

#endif
