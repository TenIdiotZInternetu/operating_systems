// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Charles University

#ifndef _DRIVERS_CSR_H_
#define _DRIVERS_CSR_H_

#define csr_quote_me_(x) #x
#define csr_quote_me(x) csr_quote_me_(x)

/** Bitmask that signifies that the MPP
 *  (machine previous privilege) is set to S (supervisor mode) in MSTATUS CSR
 */
#define CSR_MSTATUS_MPP_S 0x800

/** Bitmask of the SIE (supervisor interrupt enable) bit in SSTATUS CSR
 *  This bit controls whether interrupts are enabled in S-mode.
 */
#define CSR_SSTATUS_SIE 0x2

#define CSR_SSTATUS_SUM 1 << 18

/** The CSR number of the MEPC (machine exception program counter) CSR */
#define CSR_MEPC 0x341
/** The CSR number of the MCAUSE (machine cause) CSR */
#define CSR_MCAUSE 0x342
/** The CSR number of the MTVAL (machine trap value) CSR */
#define CSR_MTVAL 0x343

/** Bitmask for enabling the CYCLE counter in the MCOUNTEREN (machine counter enable) CSR */
#define CSR_MCOUNTEREN_CY 0x1

/** Exception code for Instruction page fault */
#define EXC_CODE_INSTRUCTION_PAGE_FAULT 12
/** Exception code for Load page fault */
#define EXC_CODE_LOAD_PAGE_FAULT 13
/** Exception code for Store/AMO page fault */
#define EXC_CODE_STORE_PAGE_FAULT 15
/** Exception code for Environment Call from U mode */
#define EXC_CODE_ECALL_U 8
/** Exception code for Illegal instruction exception */
#define EXC_CODE_ILLEGAL_INSTRUCTION 2

/** Interrupt number for Supervisor Timer Interrupt */
#define INTERRUPT_NUMBER_SUPERVISOR_TIMER_INTERRUPT 5
/** Exception code for Supervisor Timer Interrupt */
#define EXC_CODE_STI (RV_INTERRUPT_BIT | INTERRUPT_NUMBER_SUPERVISOR_TIMER_INTERRUPT)

/** The bitmask for the STIE (supervisor timer interrupt enable) of the SIE (supervisor interrupt enable) CSR */
#define CSR_SIE_STIE (1 << INTERRUPT_NUMBER_SUPERVISOR_TIMER_INTERRUPT)

/** The bitmask for the STIE (supervisor timer interrupt enable) of the MIE (machine interrupt enable) CSR */
#define CSR_MIE_STIE (CSR_SIE_STIE)

/** Instruction page fault bitmask in MEDELEG (machine exception delegation) CSR */
#define CSR_MEDELEG_IPF_MASK (1 << EXC_CODE_INSTRUCTION_PAGE_FAULT)
/** Load page fault bitmask in MEDELEG (machine exception delegation) CSR */
#define CSR_MEDELEG_LPF_MASK (1 << EXC_CODE_LOAD_PAGE_FAULT)
/** Store page fault bitmask in MEDELEG (machine exception delegation) CSR */
#define CSR_MEDELEG_SPF_MASK (1 << EXC_CODE_STORE_PAGE_FAULT)
/** User mode environment call bitmask in MEDELEG (machine exception delegation) CSR */
#define CSR_MEDELEG_ECALL_U (1 << EXC_CODE_ECALL_U)
/** Illegal instruction exception bitmask in MEDELEG (machine exception delegation) CSR */
#define CSR_MEDELEG_II (1 << EXC_CODE_ILLEGAL_INSTRUCTION)
/** Bitmask of all the delegated exceptions */
#define CSR_MEDELEG_MASK (CSR_MEDELEG_IPF_MASK | CSR_MEDELEG_LPF_MASK | CSR_MEDELEG_SPF_MASK | CSR_MEDELEG_ECALL_U | CSR_MEDELEG_II)

/** Number of custom SCYCLECMP CSR */
#define CSR_SCYCLECMP 0x5C0

/** Bit that differentiates if a trap (value in scause) was caused by an interrupt or an exception*/
#define RV_INTERRUPT_BIT 0x80000000

#ifndef __ASSEMBLER__ /* Don't include the following in Assemler */

#include <debug.h>
#include <mm/self.h>
#include <types.h>

/** Macro which returns the value of the specified CSR
 *
 * @param csr CSR to read (number or name).
 */
#define csr_read(csr) \
    ({ \
        unative_t __result; \
        __asm__ volatile( \
                "csrr %0, " csr_quote_me(csr) "\n" \
                : "=r"(__result)); \
        __result; \
    })

/** Macro which writes the specied value to the given CSR and returns the previous value of the CSR.
 *
 * eg: before:    mie = 0b1100;
 *     x = csr_write(mie, 0b1010);
 *     after: x = 0b1100, mie = 0b1010;
 *
 * @param csr CSR to write (number or name).
 * @param value The value to write into the CSR.
 * @returns The previous value of the CSR.
 */
#define csr_write(csr, value) ({ \
    unative_t __result; \
    unative_t __value = value; \
    __asm__ volatile( \
            "csrrw %0, " csr_quote_me(csr) ", %1\n" \
            : "=r"(__result) \
            : "r"(__value)); \
    __result; \
})

/** Macro which sets (writes 1 to) the bits that are set (logical 1) in the specied value of the given CSR and returns the previous value of the CSR.
 *
 * eg: before:    mie = 0b1100;
 *     x = csr_set(mie, 0b1010);
 *     after: x = 0b1100, mie = 0b1110;
 *
 * @param csr CSR to set (number or name).
 * @param value The bitmask of which bits should be set to 1.
 * @returns The previous value of the CSR.
 */
#define csr_set(csr, value) ({ \
    unative_t __result; \
    unative_t __value = value; \
    __asm__ volatile( \
            "csrrs %0, " csr_quote_me(csr) ", %1\n" \
            : "=r"(__result) \
            : "r"(__value)); \
    __result; \
})

/** Macro which clears (writes 0 to) the bits that are set (logical 1) in the specied value of the given CSR and returns the previous value of the CSR .
 *
 * eg: before: mie = 0b1100;
 *     x = csr_clear(mie, 0b1010);
 *     after: x = 0b1100, mie = 0b0100;
 *
 * @param csr CSR to clear (number or name).
 * @param value The bitmask of which bits should be reset to 0.
 * @returns The previous value of the CSR.
 */
#define csr_clear(csr, value) ({ \
    unative_t __result; \
    unative_t __value = value; \
    __asm__ volatile( \
            "csrrc %0, " csr_quote_me(csr) ", %1\n" \
            : "=r"(__result) \
            : "r"(__value)); \
    __result; \
})

/** Tells whether some interrupt is pending.
 *
 * @param scause_value Value of the scause register.
 * @return Exception code.
 */
static inline unsigned int csr_scause_is_interrupt_pending(unative_t scause_value) {
    return scause_value & RV_INTERRUPT_BIT;
}

static inline bool csr_scause_is_page_fault(unative_t scause_value) {
    unative_t exc_code = scause_value & (~RV_INTERRUPT_BIT);
    printk("EXC: %d\n", exc_code);
    return exc_code == EXC_CODE_INSTRUCTION_PAGE_FAULT
            || exc_code == EXC_CODE_STORE_PAGE_FAULT
            || exc_code == EXC_CODE_LOAD_PAGE_FAULT;
}

#endif
#endif
