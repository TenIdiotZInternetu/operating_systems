// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Charles University

#ifndef _DRIVERS_MMU_H
#define _DRIVERS_MMU_H

#include <drivers/csr.h>

static inline unative_t mmu_get_current_asid(void) {
    return (csr_read(satp) >> 22) & 0x1FF;
}

#endif
