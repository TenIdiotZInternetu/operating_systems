// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/syscall.h>
#include <stdarg.h>
#include <stdio.h>

/** Print single character to console.
 *
 * @param c Character to print.
 * @return Character written as an unsigned char cast to an int.
 */
int putchar(int c) {
    __SYSCALL1(SYSCALL_PUTCHAR, (unative_t)c);
    return (unsigned char)c;
}

/** Prints the given string to console, terminating it with a newline.
 *
 * @param str String to print.
 * @return Number of printed characters.
 */
int puts(const char* str) {
    int count = 1; // For new-line
    while (*str != 0) {
        putchar(*str);
        str++;
        count++;
    }
    putchar('\n');
    return count;
}

void write(const void* buffer, size_t count) {
    __SYSCALL2(SYSCALL_WRITE, (unative_t)buffer, (unative_t)count);
}
