// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Charles University

#ifndef _DRIVERS_SV32_H_
#define _DRIVERS_SV32_H_

#include <debug.h>
#include <drivers/csr.h>
#include <types.h>

/** Set SATP register to new value. */
static inline void sv32_set_satp(unative_t val) {
    csr_write(satp, val);
}

/** Set SATP register to new value and execute complete TLB shootdown.
 *
 * This function calls SFENCE.VMA x0, x0, i.e. it clears the whole TLB.
 *
 * Consider using a modified version that invalidates only certain ASID
 * for better performance.
 */
static inline void sv32_set_satp_tlb_shootdown(unative_t val) {
    csr_write(satp, val);
    __asm__ volatile("sfence.vma x0, x0\n");
}

/** Perform complete TLB shootdown.
 *
 * This function calls SFENCE.VMA x0, x0, i.e. it clears the whole TLB.
 *
 * Consider using a modified version that invalidates only certain ASID
 * for better performance.
 */
static inline void sv32_tlb_shootdown(void) {
    __asm__ volatile("sfence.vma x0, x0\n");
}

#endif
