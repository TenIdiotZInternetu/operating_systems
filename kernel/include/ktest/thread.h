// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _KTEST_THREAD_H
#define _KTEST_THREAD_H

#include <debug.h>
#include <proc/thread.h>

#define ktest_thread_join(thread, retval) \
    ((dprintk("Joining thread %pT...\n", (thread))), thread_join((thread), (retval)))

#define ktest_thread_join_checked(thread, retval) \
    do { \
        errno_t ___thread_join_err = ktest_thread_join((thread), (retval)); \
        ktest_assert_errno(___thread_join_err, "thread_join(" #thread ")"); \
    } while (0)

#define ktest_thread_create_checked(thread_out, entry_func, entry_arg, flags, name) \
    do { \
        errno_t ___thread_create_err = thread_create( \
                (thread_out), (entry_func), (entry_arg), (flags), (name)); \
        ktest_assert_errno(___thread_create_err, \
                "thread_create(" #thread_out ", " #entry_func "(" #entry_arg "))"); \
        dprintk("Created thread %pT (entry %s(%s), flags %d).\n", \
                *(thread_out), #entry_func, #entry_arg, (flags)); \
    } while (0)

#define ktest_thread_create_new_as_checked(thread_out, entry_func, entry_arg, flags, name, as_size) \
    do { \
        errno_t ___thread_create_err = thread_create_new_as( \
                (thread_out), (entry_func), (entry_arg), (flags), (name), (as_size)); \
        ktest_assert_errno(___thread_create_err, \
                "thread_create_new_as(" #thread_out ", " #entry_func "(" #entry_arg "), AS = " #as_size " B)"); \
        dprintk("Created thread %pT (entry %s(%s), flags %d, AS size %u).\n", \
                *(thread_out), #entry_func, #entry_arg, (flags), (as_size)); \
    } while (0)

static volatile int ktest_thread_blackhole = 0;

static inline void ktest_thread_busy_wait(int loops) {
    for (int i = 0; i < loops; i++) {
        ktest_thread_blackhole += i;
    }
}

static inline void ktest_thread_busy_wait_random(int max_wait) {
    register uintptr_t sp __asm__("sp");
    int rand = (sp >> 10) % max_wait;
    ktest_thread_busy_wait(rand);
}

#endif
