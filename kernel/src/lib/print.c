// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "mm/heap.h"
#include <adt/list.h>
#include <debug.h>
#include <drivers/printer.h>
#include <lib/print.h>
#include <lib/stdarg.h>
#include <types.h>

const char ESCAPE_CHAR = '%';

const char NULL_PTR_PRINT[] = "(nil)";
const char NULL_STR_PRINT[] = "(null)";

const uint8_t OPERATION_SIZE = 4;
const uint8_t BYTES_IN_BLOCK = 4;
const uint8_t PRINT_FUNC_OPERATIONS = 4;

const char SPECIFIER_CHAR = 'c';
const char SPECIFIER_DECIMAL = 'd';
const char SPECIFIER_UINT = 'u';
const char SPECIFIER_LOWER_HEX = 'x';
const char SPECIFIER_UPPER_HEX = 'X';
const char SPECIFIER_STRING = 's';
const char SPECIFIER_BYTE = 'b';
const char SPECIFIER_BYTE_BLOCK = 'B';
const char SPECIFIER_POINTER = 'p';

const char SPECIFIER_FOLLOWUP_LIST = 'L';
const char SPECIFIER_FOLLOWUP_FUNC = 'F';
const char SPECIFIER_FOLLOWUP_HEAP = 'A';

char digit_to_char(uint8_t digit, bool uppercase);
void print_string(const char* string);
void print_num(uint32_t num, uint8_t base, bool uppercase, uint8_t zero_padding);
void print_int(int32_t n);
void print_uint(uint32_t n);
void print_hex(uint32_t n, bool uppercase);
void print_pointer(void* p);
void print_bytes(uint8_t* address, size_t count);
void print_func(void* f, size_t operations);
void print_list(list_t* list);
void resolve_specifier(const char** format_it, va_list* args);

char digit_to_char(uint8_t digit, bool uppercase) {
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

void print_string(const char* string) {
    if (string == NULL) {
        print_string(NULL_STR_PRINT);
        return;
    }

    while (*string != '\0') {
        printer_putchar(*string++);
    }
}

// pls dont use base over 36, thx
void print_num(uint32_t num, uint8_t base, bool uppercase, uint8_t zero_padding) {
    uint32_t div = 1;
    bool paddingReached = 1 >= zero_padding;
    bool maxDivReached = num / div < base;

    for (uint8_t digits = 2; !paddingReached || !maxDivReached; digits++) {
        div *= base;
        paddingReached = digits >= zero_padding;
        maxDivReached = num / div < base;
    }

    for (; div > 0; div /= base) {
        const uint8_t digit = (num / div) % base;
        const char digitChar = digit_to_char(digit, uppercase);
        printer_putchar(digitChar);
    }
}

void print_int(int32_t n) {
    uint32_t unsigned_n = n;

    if (n < 0) {
        printer_putchar('-');
        unsigned_n = n * -1;
    }

    print_num(unsigned_n, 10, false, 0);
}

void print_uint(uint32_t n) {
    print_num(n, 10, false, 0);
}

void print_hex(uint32_t n, bool uppercase) {
    print_num(n, 16, uppercase, 0);
}

void print_pointer(void* p) {
    uint32_t p_val = (uint32_t)p;
    if (p_val == 0) {
        printk(NULL_PTR_PRINT);
    } else {
        printk("0x%x", p_val);
    }
}

// Prints bytes in adress ascending order (cause little endian ig)
void print_bytes(uint8_t* address, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        uint8_t* byte_adress = address + count - i - 1;
        print_num(*byte_adress, 16, false, 2);
    }
}

void print_func(void* func, size_t operations) {
    printk("%p[%B", func, func);
    uint8_t* func_byte_addr = func;

    for (size_t i = 1; i < operations; i++) {
        uint8_t* op_address = func_byte_addr + i * OPERATION_SIZE;
        printk(" %B", op_address);
    }

    printk("]");
}

void print_list(list_t* list) {
    printk("%p[", list);

    if (list_is_empty(list)) {
        printk("empty]");
        return;
    }

    link_t* head = &(list->head);
    link_t* first_link = head->next;
    printk("%u: %p", list_get_size(list), first_link);

    link_t* link = first_link->next;
    while (link != head) {
        printk("-%p", link);
        link = link->next;
    }

    printk("]");
}

static void print_heap_block(void* ptr) {
    block_head_t* block_head = (block_head_t*)ptr - 1;
    printk("(%p)[%c][size: %u]",
            ptr,
            block_head->is_free ? ' ' : 'X',
            block_head->size);
}

void resolve_specifier(const char** format_it, va_list* args) {
    const char c = *++*format_it;

    if (c == SPECIFIER_CHAR) {
        printer_putchar(va_arg(*args, int)); // chars are promoted to int
    } else if (c == SPECIFIER_DECIMAL) {
        print_int(va_arg(*args, int32_t));
    } else if (c == SPECIFIER_UINT) {
        print_uint(va_arg(*args, uint32_t));
    } else if (c == SPECIFIER_LOWER_HEX) {
        print_hex(va_arg(*args, uint32_t), false);
    } else if (c == SPECIFIER_UPPER_HEX) {
        print_hex(va_arg(*args, uint32_t), true);
    } else if (c == SPECIFIER_STRING) {
        print_string(va_arg(*args, const char*));
    } else if (c == SPECIFIER_BYTE) {
        print_bytes(va_arg(*args, void*), 1);
    } else if (c == SPECIFIER_BYTE_BLOCK) {
        print_bytes(va_arg(*args, void*), BYTES_IN_BLOCK);
    } else if (c == SPECIFIER_POINTER) {
        const char next_c = *++*format_it;

        if (next_c == '\0') {
            print_pointer(va_arg(*args, void*));
            return;
        } else if (next_c == SPECIFIER_FOLLOWUP_HEAP) {
            print_heap_block(va_arg(*args, void*));
        } else if (next_c == SPECIFIER_FOLLOWUP_LIST) {
            print_list(va_arg(*args, list_t*));
        } else if (next_c == SPECIFIER_FOLLOWUP_FUNC) {
            print_func(va_arg(*args, void*), PRINT_FUNC_OPERATIONS);
        } else {
            print_pointer(va_arg(*args, void*));
            printer_putchar(next_c);
        }
    } else {
        printer_putchar(c);
    }

    ++*format_it;
}

/** Simplified printf.
 *
 * @param format Formatting string.
 * @param ... Arguments for the format string.
 */
void printk(const char* format, ...) {
    const char* it = format;

    va_list args;
    va_start(args, format);

    while (*it != '\0') {
        const bool isLastChar = *(it + 1) == '\0';
        if (*it == ESCAPE_CHAR && !isLastChar) {
            resolve_specifier(&it, &args);
            continue;
        }

        printer_putchar(*it);
        it++;
    }
}

/** Prints given string to console, terminating it with newline.
 *
 * @param s String to print.
 */
void puts(const char* s) {
    printk("%s\n", s);
}
