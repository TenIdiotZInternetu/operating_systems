// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Charles University

/*
 * Tests that as_get_root_page_table returns a reasonably filled-in structure.
 *
 * This test does not create any new threads and only creates address spaces
 * without binding them to any thread. Therefore, the address space is never
 * expected to be activated (i.e. the translation will never be really used).
 *
 * This means that this test is usable even before the processor is switched
 * to Sv32 mode (i.e. not BARE mode). In other words, this test can be used
 * to check that page tables are constructed correctly even before we enable
 * paging at all.
 *
 * The test intentionally uses magic numbers so that the test cannot be
 * easily used instead of the documentation of RISC-V page translation
 * process.
 */

#include <ktest.h>
#include <ktest/thread.h>
#include <mm/as.h>
#include <proc/thread.h>

/* Maximum size of the lower 2G where the userland will expect the mapping. */
#define MAX_PAGES 4095
#define MIN_PAGES 2

static uintptr_t backing_frames[MAX_PAGES];

static inline int get_bit(unsigned int value, int bit) {
    return (value >> bit) & 0x1;
}

#define ktest_assert_valid_page_table_address(addr) \
    do { \
        ktest_assert((addr) != 0, "Page table address cannot be 0."); \
        ktest_assert(((addr) % PAGE_SIZE) == 0, "Page table address must be page-aligned (got 0x%x).", (addr)); \
        ktest_assert(((addr) >= 0x80000000) && ((addr) < 0xA0000000), \
                #addr " 0x%x not in [0x80000000, 0xA0000000)", (addr)); \
    } while (0)

#define ktest_assert_uxwr_valid(uxwr) \
    do { \
        unsigned int __xwr = uxwr & 0x7; \
        ktest_assert((__xwr != 2) && (__xwr != 6), "xwr flags set to reserved value (%d%d%d)", \
                get_bit(__xwr, 2), get_bit(__xwr, 1), get_bit(__xwr, 0)); \
    } while (0)

static int resolve_mapping(uintptr_t root_pt, uintptr_t virtual_page, uintptr_t* physical_page, unsigned int* uxwr) {
    // We assume that all frames are from kernel memory and that kernel is
    // identically mapped.
    ktest_assert_valid_page_table_address(root_pt);

    unative_t* page_table = (unative_t*)root_pt;

    unsigned int pn1 = virtual_page >> 22;
    unsigned int pn0 = (virtual_page >> 12) & ((1 << 10) - 1);

    dprintk("[root_pt = 0x%x] virtual = 0x%x  pn1 = 0x%x  pn0 = 0x%x \n", root_pt, virtual_page, pn1, pn0);

    unative_t entry = page_table[pn1];
    if ((entry & 1) == 0) {
        return ENOENT;
    }
    unsigned int flags_uxwr = (entry >> 1) & 0xf;
    dprintk("[root_pt = 0x%x] entry = 0x%x  flags = %d\n", root_pt, entry, flags_uxwr);
    ktest_assert_uxwr_valid(flags_uxwr);
    if ((flags_uxwr & 0x7) != 0) {
        uintptr_t phys = ((entry >> 10) + pn0) * 4096;
        dprintk("[root_pt = 0x%x] mega (huge) page %u: 0x%x => 0x%x\n", root_pt, pn1, entry, phys);
        uintptr_t ppn0 = (entry >> 10) & 0x3ff;
        ktest_assert(ppn0 == 0, "Mega page (virt = 0x%x, entry = 0x%x) must be naturally aligned but ppn0 is 0x%x", virtual_page, entry, ppn0);
        *uxwr = flags_uxwr;
        *physical_page = phys;
        return EOK;
    }

    uintptr_t nested_pt = (entry >> 10) * 4096;
    ktest_assert_valid_page_table_address(nested_pt);
    unative_t* nested_page_table = (unative_t*)nested_pt;

    entry = nested_page_table[pn0];
    if ((entry & 1) == 0) {
        return ENOENT;
    }
    flags_uxwr = (entry >> 1) & 0xf;
    ktest_assert_uxwr_valid(flags_uxwr);
    if ((flags_uxwr & 0x7) != 0) {
        *uxwr = flags_uxwr;
        *physical_page = (entry >> 10) * 4096;
        return EOK;
    }

    ktest_assert(false, "Too many nestings detected.");
    return ENOENT;
}

static void check_kernel_mapping(uintptr_t root_page_table, uintptr_t kernel_page) {
    uintptr_t phys = 0;
    unsigned int flags_uxwr = 0;
    int err = resolve_mapping(root_page_table, kernel_page, &phys, &flags_uxwr);
    ktest_assert(err == EOK,
            "Mapping should exist for 0x%x but got %d (%s)",
            kernel_page, err, errno_as_str(err));
    ktest_assert(phys == kernel_page,
            "Kernel memory should be identity mapped (page 0x%x mapped to 0x%x)",
            kernel_page, phys);
    ktest_assert(flags_uxwr == 0x7,
            "Kernel memory must be XWR but not U (got 0b%d%d%d%d, expecting 0b0111)",
            get_bit(flags_uxwr, 3), get_bit(flags_uxwr, 2), get_bit(flags_uxwr, 1), get_bit(flags_uxwr, 0));
}

void kernel_test(void) {
    ktest_start("as/pagetable");

    for (size_t size = PAGE_SIZE * MIN_PAGES; size < PAGE_SIZE * MAX_PAGES; size += PAGE_SIZE) {
        printk("Will check AS (size = %uB, %u pages)\n", size, size / PAGE_SIZE);
        as_t* as = as_create(size, 0);
        if (as == NULL) {
            break;
        }
        uintptr_t root_page_table = as_get_root_page_table(as);
        ktest_assert_valid_page_table_address(root_page_table);

        // Check mappings of kernel pages (at least 1MB of memory)
        for (uintptr_t kernel_page = 0x80000000; kernel_page < 0x80100000; kernel_page += PAGE_SIZE) {
            check_kernel_mapping(root_page_table, kernel_page);
        }
        // Check mapping of the printer device
        check_kernel_mapping(root_page_table, 0x90000000);

        // Check user page mappings (checking whole 2G takes too much time, hence
        // we check only up to 0x01000000 which finishes reasonably fast)
        for (uintptr_t user_page = 0; user_page < 0x01000000; user_page += PAGE_SIZE) {
            uintptr_t phys = 0;
            unsigned int flags_uxwr = 0;
            int err = resolve_mapping(root_page_table, user_page, &phys, &flags_uxwr);
            if (user_page < PAGE_NULL_COUNT * PAGE_SIZE) {
                ktest_assert(err == ENOENT,
                        "Mapping should not exist for null page 0x%x but got %d (%s)",
                        user_page, err, errno_as_str(err));
            } else if (user_page >= size + PAGE_NULL_COUNT * PAGE_SIZE) {
                ktest_assert(err == ENOENT,
                        "Mapping should not exist for too high page 0x%x but got %d (%s)",
                        user_page, err, errno_as_str(err));
            } else {
                ktest_assert(err == EOK,
                        "Mapping should exist for 0x%x but got %d (%s)",
                        user_page, err, errno_as_str(err));
                ktest_assert(phys != user_page,
                        "User memory cannot be identity mapped (page 0x%x mapped to 0x%x)",
                        user_page, phys);
                ktest_assert(flags_uxwr == 0xf,
                        "User memory must be UXWR but got 0b%d%d%d%d (expecting 0b1111).",
                        get_bit(flags_uxwr, 3), get_bit(flags_uxwr, 2), get_bit(flags_uxwr, 1), get_bit(flags_uxwr, 0));
                backing_frames[user_page / PAGE_SIZE] = phys;
            }
        }

        // Check each user page is backed by a different frame
        for (size_t i = PAGE_NULL_COUNT; i < size / PAGE_SIZE; i++) {
            for (size_t j = i + 1; j < size / PAGE_SIZE; j++) {
                ktest_assert(backing_frames[i] != backing_frames[j],
                        "Frames backing user pages cannot overlap (pages %u and %u backed by 0x%x)",
                        i, j, backing_frames[i]);
            }
        }
        as_destroy(as);
    }

    ktest_passed();
}
