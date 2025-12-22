// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _EXC_H
#define _EXC_H

#include <drivers/csr.h>
#include <exc/context.h>
#include <types.h>

void handle_exception_general(exc_context_t* exc_context);

unative_t get_next_satp(void);

/** Disables all interrupts (on the CPU) and returns the previous state.
 *
 * Saves and returns the value of bit 1 (SIE) of sstatus CSR prior
 * to clearing it (and thus disabling interrupts).
 *
 * This function also performs a full memory barrier as it is expected
 * that it will be used for synchronization purposes.
 */
static inline bool interrupts_disable(void) {
    unative_t status = csr_clear(sstatus, CSR_SSTATUS_SIE);
    __sync_synchronize();
    return (status & CSR_SSTATUS_SIE) != 0;
}

/** Restores the interrupt-enable state (on the CPU).
 *
 * Sets bit 1 (SIE) of the sstatus CSR to the given value.
 * Typically used with the return value of interrupts_disable()
 * to perform a complementary operation.
 *
 * This function also performs a full memory barrier as it is expected
 * that it will be used for synchronization purposes.
 */
static inline void interrupts_restore(bool enable) {
    __sync_synchronize();
    if (enable) {
        csr_set(sstatus, CSR_SSTATUS_SIE);
    } else {
        csr_clear(sstatus, CSR_SSTATUS_SIE);
    }
}

static inline void set_SUM() {
    csr_set(sstatus, CSR_SSTATUS_SUM);
}

#endif
