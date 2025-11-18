// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _DRIVERS_TIMER_H_
#define _DRIVERS_TIMER_H_

#include <drivers/csr.h>
#include <types.h>

static inline void timer_interrupt_after(unative_t cycles) {
    csr_write(CSR_SCYCLECMP, csr_read(cycle) + cycles);
}

#endif
