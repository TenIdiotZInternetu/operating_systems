// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H

#include <stddef.h>

int putchar(int c);
int puts(const char* s);
int printf(const char* format, ...);

#endif
