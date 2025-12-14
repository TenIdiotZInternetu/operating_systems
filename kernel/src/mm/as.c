// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/printer.h>
#include <errno.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mm/heap.h>

#define PTE_VALID_BIT 0
#define PTE_READ_BIT 1
#define PTE_WRITE_BIT 2
#define PTE_EXECUTE_BIT 3
#define PTE_USERMODE_BIT 4
#define PTE_GLOBAL_BIT 5
#define PTE_ACCESSED_BIT 6
#define PTE_DIRTY_BIT 7

#define PTE_VALIDITY_BITMASK 0x1
#define PTE_PPN_MASK 0xFFFFF000

/** Initializes support for address spaces.
 *
 * Called once at system boot.
 */
void as_init(void) {
}

/**
 * Allocates frames for VMA pages
 * @param lvl2_page_table address of the lvl2 page table where to add VMA entries
 * @param first_table Whether we are need to leave unmapped nullpages
 * @param VMA_pages_to_be_mapped VMA pages that have no mapping yet
 * @return Error code
 * @retval EOK Frame succesfully allocated for every page
 * @retval ENOMEM Not enough memory
 */
static errno_t _map_VMA_impl(uint32_t* lvl2_page_table, bool first_table, size_t* VMA_pages_to_be_mapped) {
    for (size_t i = 0; i < PAGES_PER_TABLE; ++i) {
        bool VMA_page = (!first_table || i > 1) && (*VMA_pages_to_be_mapped > 0);
        if (VMA_page) {
            uintptr_t page_physical_address;
            errno_t err = frame_alloc(1, &page_physical_address);
            if (err == ENOMEM) {
                return err;
            }
            _add_lvl2_pte(lvl2_page_table, i, page_physical_address, true);
            --(*VMA_pages_to_be_mapped);
        }
        else {
            lvl2_page_table[i] = 0;
        }
    }
    return EOK;
}

/**
 * Allocates frames for lvl1 entries with pointers to lvl2 tables with VMA mapping, calls _map_VMA_impl
 * @param as Address space for which we create the mapping
 * @return Error code
 * @retval EOK Frame succesfully allocated for every VMA page
 * @retval ENOMEM Not enough memory
 */
static errno_t _map_VMA(as_t* as) {
    size_t VMA_pages_to_be_mapped = as->size / PAGE_SIZE;
    bool first_table = true;
    uintptr_t lvl1_entry_index = USERSPACE_ADDRESS;

    while (VMA_pages_to_be_mapped > 0) {
        uintptr_t lvl2_table_address;
        errno_t err = frame_alloc(1, &lvl2_table_address);
        if (err == ENOMEM) {
            return err;
        }
        _add_lvl1_pte((uint32_t*)as->root_table_addr, lvl1_entry_index, lvl2_table_address, false, true);

        errno_t err = _map_VMA_impl((uint32_t*)lvl2_table_address, first_table, &VMA_pages_to_be_mapped);
        if (err == ENOMEM) {
            return err;
        }
        first_table = false;
        ++lvl1_entry_index;
    }
    return EOK;
}

/**
 * Creates mapping for kernel
 * @param page_table_address Address of (root) page table to which we add the entry (megapage)
 */
static void _map_kernel(uint32_t* page_table_address) {
    assert(KERNEL_ADDRESS % MEGAPAGE_SIZE == 0);
    uintptr_t lvl1_entry_index = KERNEL_ADDRESS / MEGAPAGE_SIZE;
    _add_lvl1_pte(page_table_address, lvl1_entry_index, lvl1_entry_index, false, false);
}

/**
 * Creates mapping for printer
 * @param page_table_address Address of (root) page table to which we add the entry (megapage)
 */
static void _map_printer(uint32_t* page_table_address) {
    assert(PRINTER_ADDRESS % MEGAPAGE_SIZE == 0);
    uintptr_t lvl1_entry_index = PRINTER_ADDRESS / MEGAPAGE_SIZE;
    _add_lvl1_pte(page_table_address, lvl1_entry_index, lvl1_entry_index, false, false);
}

/**
 * Fills the entries that need to be mapped
 * @param as Address space for which we are creating mapping
 * @return Error code
 * @retval EOK Frame succesfully allocated for every VMA page
 * @retval ENOMEM Not enough memory
 */
static errno_t _map_pages(as_t* as) {
    errno_t err = _map_VMA(as);
    if (err == ENOMEM) {
        return err;
    }
    _map_kernel((uint32_t*)as->root_table_addr);
    _map_printer((uint32_t*)as->root_table_addr);
    return EOK:
}

/**
 * Initializes page table of new address space, including frame allocation
 * @param as Address space for which we are initializing the page table
 * @return Error code
 * @retval EOK Frame succesfully allocated for every VMA page
 * @retval ENOMEM Not enough memory 
 */
static errno_t _page_table_init(as_t* as) {
    // invalidate page table addresses
    for (uintptr_t i = 0; i < PAGES_PER_TABLE; ++i) {
        ((uint32_t*)as->root_table_addr)[i] = 0;
    }

    errnot_t err = _map_pages(as);
    return err;
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

    address_space->size = size;
    errnot_t err = _page_table_init(address_space);
    if (err == ENOMEM) {
        return NULL;
    }
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
        bool entry_is_valid = lvl1_entry & PTE_VALIDITY_BITMASK;

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
static void _add_pte(uint32_t* page_table_address, uintptr_t entry_index, uintptr_t phys_addr, bool is_leaf, bool userspace_access) {
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
 *  @param lvl1_page_table_address Address of the page table where to add the entry.
 *  @param entry_index Index of the entry we want to add in the page table
 *  @param phys_addr Physical address the entry should point to
 *  @param is_leaf Whether the entry should be a leaf entry
 *  @param userspace_access Whether the page should be accessible in userspace
 */
static void _add_lvl1_pte(uint32_t* lvl1_page_table_address, uintptr_t entry_index, uintptr_t phys_addr, bool is_leaf, bool userspace_access) {
    _add_pte(lvl1_page_table_address, entry_index, phys_addr, is_leaf, userspace_access);
}

/**
 *  Adds an entry to the second level page table, and sets it to valid.
 *  If an entry pointing to the second level table doesn't exist in the root table,
 *  it is created as well.
 *
 *  @param lvl2_page_table_address Address of the page table where to add the entry.
 *  @param entry_index Index of the entry we want to add in the page table
 *  @param phys_addr Physical address the entry should point to
 *  @param userspace_access Whether the page should be accessible in userspace
 */
static void _add_lvl2_pte(uint32_t* lvl2_page_table_address, uintptr_t entry_index, uintptr_t phys_addr, bool userspace_access) {
    _add_pte(lvl2_page_table_address, entry_index, phys_addr, false, userspace_access);
}