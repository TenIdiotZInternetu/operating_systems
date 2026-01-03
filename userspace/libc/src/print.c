// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/syscall.h>
#include <stdarg.h>
#include <stdio.h>

#define PRINTF_BUFFER_SIZE 256

const char ESCAPE_CHAR = '%';
const char NULL_PTR_PRINT[] = "(nil)";
const char NULL_STR_PRINT[] = "(null)";

const char SPECIFIER_CHAR = 'c';
const char SPECIFIER_DECIMAL = 'd';
const char SPECIFIER_UINT = 'u';
const char SPECIFIER_LOWER_HEX = 'x';
const char SPECIFIER_UPPER_HEX = 'X';
const char SPECIFIER_STRING = 's';
const char SPECIFIER_POINTER = 'p';

static char printf_char_buffer[PRINTF_BUFFER_SIZE];
static size_t curr_printf_char_buffer_pos = 0;

static void _flush(void);
static void _format_char(char c);
static void _format_char_no_bound_check(char c);
static void _format_string(const char* string);
static void _format_num(uint32_t num, uint8_t base, bool uppercase, uint8_t zero_padding)
static void _format_hex(uint32_t n, bool uppercase);
static char _digit_to_char(uint8_t digit, bool uppercase);

static bool _is_before_last_buffer_pos(void) {
    return (curr_printf_char_buffer_pos < PRINTF_BUFFER_SIZE - 1);
}

static char _digit_to_char(uint8_t digit, bool uppercase) {
    if (digit < 10) {
        return '0' + digit;
    } else {
        char letter_digit = 'A' + digit - 10;
        if (uppercase) {
            return letter_digit;
        } else {
            const uint8_t ascii_case_offset = 32;
            return letter_digit + ascii_case_offset;
        }
    }
}

static void _format_char_no_bound_check(char c) {
    printf_char_buffer[curr_printf_char_buffer_pos] = c;
    ++curr_printf_char_buffer_pos;
}

static void _format_char(char c) {
    if (_is_before_last_buffer_pos()) {
        _format_char_no_bound_check(c);
    }
}

static void _format_string(const char* string) {
    if (string == NULL) {
        _format_string(NULL_STR_PRINT);
        return;
    }

    for (size_t i = 0; (string[i] != '\0') && _is_before_last_buffer_pos(); ++i) {
        printf_char_buffer[curr_printf_char_buffer_pos] = string[i];
        ++curr_printf_char_buffer_pos;
    }
}

static void _format_num(uint32_t num, uint8_t base, bool uppercase, uint8_t zero_padding) {
    uint32_t div = 1;
    bool paddingReached = 1 >= zero_padding;
    bool maxDivReached = num / div < base;

    for (uint8_t digits = 2; !paddingReached || !maxDivReached; digits++) {
        div *= base;
        paddingReached = digits >= zero_padding;
        maxDivReached = num / div < base;
    }

    for (; div > 0; div /= base) {
        if (!_is_before_last_buffer_pos()) break;
        const uint8_t digit = (num / div) % base;
        const char digitChar = _digit_to_char(digit, uppercase);
        _format_char_no_bound_check(digitChar);
    }
}

static void _format_int(int32_t n) {
    uint32_t unsigned_n = n;

    if (n < 0) {
        _format_char('-');
        unsigned_n = n * -1;
    }

    _format_num(unsigned_n, 10, false, 0);
}

static void _format_uint(uint32_t n) {
    _format_num(n, 10, false, 0);
}

static void _format_hex(uint32_t n, bool uppercase) {
    _format_num(n, 16, uppercase, 0);
}

static void _format_pointer(void* p) {
    uint32_t p_val = (uint32_t)p;
    if (p_val == 0) {
        _format_string(NULL_PTR_PRINT);
    } else {
        _format_string("0x");
        _format_hex(p_val, false);
    }
}

static void _resolve_specifier(const char** format_it, va_list* args) {
    const char c = *++*format_it;

    if (c == SPECIFIER_CHAR) {
        _format_char((char)va_arg(*args, int));
    } else if (c == SPECIFIER_DECIMAL) {
        _format_int(va_arg(*args, int32_t));
    } else if (c == SPECIFIER_UINT) {
        _format_uint(va_arg(*args, uint32_t));
    } else if (c == SPECIFIER_LOWER_HEX) {
        _format_hex(va_arg(*args, uint32_t), false);
    } else if (c == SPECIFIER_UPPER_HEX) {
        _format_hex(va_arg(*args, uint32_t), true);
    } else if (c == SPECIFIER_STRING) {
        _format_string(va_arg(*args, const char*));
    } else if (c == SPECIFIER_POINTER) {
        _format_pointer(va_arg(*args, void*));
    } else {
        _format_char(c);
    }

    ++*format_it;
}

static void _flush(void) {
    if (curr_printf_char_buffer_pos > 0) {
        write(printf_char_buffer, curr_printf_char_buffer_pos);
    }
    curr_printf_char_buffer_pos = 0;
}

/** Print single character to console.
 *
 * @param c Character to print.
 * @return Character written as an unsigned char cast to an int.
 */
int putchar(int c) {
    __SYSCALL1(SYSCALL_PUTCHAR, (unative_t)c);
    return (unsigned char)c;
}

/**
 * Issues a system call that writes buffer's content to console.
 */
void write(const void* buffer, size_t count) {
    return (ssize_t)__SYSCALL2(SYSCALL_WRITE,(unative_t)buffer, (unative_t)count);
}

/** Prints the given formatted string to console.
 *
 * @param format printf-style formatting string.
 * @return Number of printed characters.
 */
int printf(const char* format, ...) {
    const char* it = format;

    va_list args;
    va_start(args, format);

    while (*it != '\0') {
        const bool isLastChar = (*(it + 1) == '\0');
        if (*it == ESCAPE_CHAR && !isLastChar) {
            _resolve_specifier(&it, &args);
            continue;
        }

        _format_char(*it);
        it++;
    }

    va_end(args);

    int printed_chars = (int)curr_printf_char_buffer_pos;
    _flush();

    return printed_chars;
}

/** Prints the given string to console, terminating it with a newline.
 *
 * @param str String to print.
 * @return Number of printed characters.
 */
int puts(const char* str) {
    return printf("%s\n", str);
}