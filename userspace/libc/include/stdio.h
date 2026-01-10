// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H

#include <stddef.h>

#define PRINTF_BUFFER_SIZE 256

int putchar(int c);
int puts(const char* s);
int printf(const char* format, ...);

void write(const void* buffer, size_t count);

#endif
