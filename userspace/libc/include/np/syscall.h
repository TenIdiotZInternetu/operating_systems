// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_NP_SYSCALL_H
#define _LIBC_NP_SYSCALL_H

#include <assert.h>
#include <np/types.h>

/*
 * Syscall wrappers for different argument counts.
 */

#define __SYSCALL1(id, p1) __syscall((id), (p1), 0, 0, 0)
#define __SYSCALL2(id, p1, p2) __syscall((id), (p1), (p2), 0, 0)
#define __SYSCALL3(id, p1, p2, p3) __syscall((id), (p1), (p2), (p3), 0)
#define __SYSCALL4(id, p1, p2, p3, p4) __syscall((id), (p1), (p2), (p3), (p4))

/** Available system calls.
 *
 * Must be kept up-to-date with kernel list.
 */
typedef enum {
    SYSCALL_PUTCHAR,
    SYSCALL_EXIT,
    SYSCALL_LAST
} syscall_t;

/** Execute a syscall.
 *
 * @param id Syscall id.
 * @param p1 First syscall argument.
 * @param p2 Second syscall argument.
 * @param p3 Third syscall argument.
 * @param p4 Fourth syscall argument.
 * @return Error code (syscall-dependent).
 */
static inline unative_t __syscall(syscall_t id, unative_t p1, unative_t p2, unative_t p3, unative_t p4) {
    assert((id >= 0) && (id < SYSCALL_LAST));

    /*
     * Pass arguments directly in registers.
     */
    register unative_t reg_a0 __asm__("a0") = id;
    register unative_t reg_a1 __asm__("a1") = p1;
    register unative_t reg_a2 __asm__("a2") = p2;
    register unative_t reg_a3 __asm__("a3") = p3;
    register unative_t reg_a4 __asm__("a4") = p4;

    __asm__ volatile(
            "ecall\n"
            : "=r"(reg_a0)
            : "r"(reg_a0),
            "r"(reg_a1),
            "r"(reg_a2),
            "r"(reg_a3),
            "r"(reg_a4)
            /*
             * This is actually a function call though C compiler may
             * not realize that.
             */
            : "ra");

    return reg_a0;
}

#endif
