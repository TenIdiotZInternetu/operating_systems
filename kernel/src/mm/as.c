// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/printer.h>
#include <errno.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mm/heap.h>

#define KERNEL_ADDRESS 0x80000000
#define PTE_SIZE_BYTES 4
#define PTE_SIZE_BITS (PTE_SIZE_BYTES * 8)

#define PTE_VALID_BIT 0
#define PTE_READ_BIT 1
#define PTE_WRITE_BIT 2
#define PTE_EXECUTE_BIT 3
#define PTE_USERMODE_BIT 4
#define PTE_GLOBAL_BIT 5
#define PTE_ACCESSED_BIT 6
#define PTE_DIRTY_BIT 7

#define PTE_VALIDITY_MASK 0x1
#define PTE_PPN_MASK 0xFFFFF000
#define PTE_VPN1_SIZE 10
#define PTE_VPN2_SIZE 10
#define PTE_VPN1_MASK ((1 << PTE_VPN1_SIZE) - 1) << (PTE_SIZE_BITS - PTE_VPN1_SIZE)
#define PTE_VPN2_MASK ((1 << PTE_VPN1_SIZE) - 1) << (PTE_SIZE_BITS - PTE_VPN1_SIZE - PTE_VPN2_SIZE)

/** Initializes support for address spaces.
 *
 * Called once at system boot.
 */
void as_init(void) {
}

/** Create new address space of given size.
 *
 * The address space must start at address (PAGE_SIZE * PAGE_NULL_COUNT).
 *
 * @param size Address space size (in bytes), aligned to PAGE_SIZE.
 * @param flags Flags (unused).
 * @return New address space.
 * @retval NULL Out of memory.
 */
as_t* as_create(size_t size, unsigned int flags) {
    as_t* address_space = (as_t*)kmalloc(sizeof(as_t));
    errno_t err = frame_alloc(1, &address_space->root_table_addr);
    if (err == ENOMEM) {
        return NULL;
    }

    address_space->size = PAGE_SIZE;
    _add_lvl1_pte(&address_space, KERNEL_ADDRESS, KERNEL_ADDRESS, true);
    _add_lvl2_pte(&address_space, PRINTER_ADDRESS, PRINTER_ADDRESS);
    return &address_space;
}

/** Get the size (in bytes) of a given address space. */
size_t as_get_size(as_t* as) {
    return as->size;
}

/**
 * Allocates frames and maps them to the given virtual address in the 2nd level page table
 *
 * @param as Address space to use
 * @param virtual_addr Virtual address to map the frames to
 * @param count Amount of frames to allocate
 * @return The physical address of the allocated frame
 * @retval NULL Out of memory
 */
uintptr_t as_alloc_page(as_t* as, uintptr_t virtual_addr, size_t count) {
    // TODO
}

/** Destroy a given address space, freeing all the memory.
 *
 * It is not expected that a user will call this function directly: an address
 * space should be destroyed when its last thread exits. Therefore one should
 * keep track of owning threads (such API is not defined by the tests and is
 * implementation specific) and when the counter reaches zero, call this
 * function.
 *
 * Note that this function is called explicitly only in unit tests for
 * the as_t structure.
 */
void as_destroy(as_t* as) {
    for (size_t i = 0; i < PAGE_SIZE; i += PTE_SIZE_BYTES) {
        // TODO solve how to get the actual entry value from the root page table
        uint32_t lvl1_entry;
        bool entry_is_valid = lvl1_entry & PTE_VALIDITY_MASK;

        if (entry_is_valid) {
            frame_free(1, lvl1_entry & PTE_PPN_MASK);
        }
    }

    kfree(as);
}

/** Get physical address of root page table.
 *
 * The function should return complete physical address, i.e. not only PPN.
 *
 * @param as Address space to use.
 * @return Physical address of root page table.
 */
uintptr_t as_get_root_page_table(as_t* as) {
    return as->root_table_addr;
}

/**
 * Assigns address space to a thread, and increases the spaces threads counter.
 *
 * @param as Address space to assign
 * @param thread Thread to assign the space to
 */
void as_add_thread(as_t* as, thread_t* thread) {
    thread->address_space = as;
    as->using_threads++;
}

/**
 * Removes address space from a thread, and decreses the spaces threads counter.
 * If there are no threads using the space left, the space is destroyed
 *
 * @param as Address space to assign
 * @param thread Thread to assign the space to
 */
void as_remove_thread(thread_t* thread) {
    as_t* as = thread->address_space;
    assert(as->using_threads != 0);

    thread->address_space = NULL;
    as->using_threads--;

    if (as->using_threads == 0) {
        as_destroy(as);
    }
}

/**
 *  Adds an entry to the first (root) level page table, and sets it to valid
 *
 *  @param page_table_address Address of the page table where to add the entry.
 *  @param entry_index Index of the entry we want to add in the page table
 *  @param phys_addr Physical address the entry should point to
 *  @param is_leaf Whether the entry should be a leaf entry
 *  @param userspace_access Whether the page should be accessible in userspace
 */
static void _add_pte(uint32_t* page_table_address, size_t entry_index, uintptr_t phys_addr, bool is_leaf, bool userspace_access) {
    uint32_t* entry = page_table_address + entry_index;
    *entry = 0;
    *entry |= 1 << PTE_VALID_BIT;
    *entry |= is_leaf << PTE_READ_BIT;
    *entry |= is_leaf << PTE_WRITE_BIT;
    *entry |= is_leaf << PTE_EXECUTE_BIT;
    *entry |= userspace_access << PTE_USERMODE_BIT;
    *entry |= phys_addr;
}

/**
 *  Adds an entry to the first (root) level page table, and sets it to valid
 *
 *  @param as Address space to use.
 *  @param virtual_addr Virtual address to map in the entry
 *  @param phys_addr Physical address the entry should point to
 *  @param is_leaf Whether the entry should be a leaf entry
 */
static void _add_lvl1_pte(as_t* as, uintptr_t virtual_addr, uintptr_t phys_addr, bool is_leaf, bool userspace_access) {
    size_t entry_index = (virtual_addr & PTE_VPN1_MASK) >> (PTE_SIZE_BITS - PTE_VPN1_SIZE);
    _add_pte((uint32_t*)as->root_table_addr, entry_index, phys_addr, is_leaf, userspace_access);
}

/**
 *  Adds an entry to the second level page table, and sets it to valid.
 *  If an entry pointing to the second level table doesn't exist in the root table,
 *  it is created as well.
 *
 *  @param as Address space to use.
 *  @param virtual_addr Virtual address to map in the entry
 *  @param phys_addr Physical address the entry should point to
 *  @retval EOK Operation successful
 *  @retval ENOMEM Out of memory
 */
static errno_t _add_lvl2_pte(as_t* as, uintptr_t virtual_addr, uintptr_t phys_addr, bool userspace_access) {
    // TODO
    // reuse _add_lvl1_pte
    return ENOIMPL;
}