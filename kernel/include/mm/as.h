// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_AS_H
#define _MM_AS_H

#include <errno.h>
#include <types.h>

/*
 * Setup amount of ASIDS that are actually supported by the hardware.
 *
 * We use a a simplification here: MSIM supports full 9 bits of ASIDs and
 * do not require our kernel to detect that.
 *
 * To speed up some of the tests we set this constant from tester.py so that
 * the implementation uses less values and thus more quickly recycles the
 * ASID numbers.
 */
#ifndef HARDWARE_ASIDS_MAX
#define HARDWARE_ASIDS_MAX 512
#endif

#define PAGE_SIZE 4096

#define PAGE_NULL_COUNT 2

typedef struct as {
    size_t size;
} as_t;

void as_init(void);
as_t* as_create(size_t size, unsigned int flags);
size_t as_get_size(as_t* as);
void as_destroy(as_t* as);
uintptr_t as_get_root_page_table(as_t* as);

#endif
