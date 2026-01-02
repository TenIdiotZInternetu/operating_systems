// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/bitmap.h>
#include <drivers/printer.h>
#include <errno.h>
#include <mm/as.h>
#include <mm/heap.h>

#define PTE_VALID_BIT 0
#define PTE_READ_BIT 1
#define PTE_WRITE_BIT 2
#define PTE_EXECUTE_BIT 3
#define PTE_USERMODE_BIT 4
#define PTE_GLOBAL_BIT 5
#define PTE_ACCESSED_BIT 6
#define PTE_DIRTY_BIT 7

#define PTE_VALIDITY_MASK 1
#define PTE_PPN_SIZE 22
#define PTE_PPN_MASK ((1 << PTE_PPN_SIZE) - 1) << (PTE_SIZE_BITS - PTE_PPN_SIZE)
#define PTE_VPN1_SIZE 10
#define PTE_VPN2_SIZE 10
#define PTE_VPN1_MASK ((1 << PTE_VPN1_SIZE) - 1) << (PTE_SIZE_BITS - PTE_VPN1_SIZE)
#define PTE_VPN2_MASK ((1 << PTE_VPN2_SIZE) - 1) << (PTE_SIZE_BITS - PTE_VPN1_SIZE - PTE_VPN2_SIZE)

#define LOADER_CODE_ADDRESS 0xF0000000

// TODO - thread_kill interrupts, deallocation of main table

/** Initializes support for address spaces.
 *
 * Called once at system boot.
 */
void as_init(void) {
}

/**
 * Invalidates all entries in new page table
 * @param table_address Address of the initialized table
 */
static void _table_init(uint32_t* table_address) {
    for (size_t i = 0; i < PAGES_PER_TABLE; ++i) {
        table_address[i] = 0;
    }
}

static size_t _lvl1_entry_index(uintptr_t virtual_addr) {
    return (virtual_addr & PTE_VPN1_MASK) >> (PTE_SIZE_BITS - PTE_VPN1_SIZE);
}

static size_t _lvl2_entry_index(uintptr_t virtual_addr) {
    return (virtual_addr & PTE_VPN2_MASK) >> (PTE_SIZE_BITS - PTE_VPN1_SIZE - PTE_VPN2_SIZE);
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
static void _add_pte(uintptr_t page_table_address, size_t entry_index, uintptr_t phys_addr, bool is_leaf, bool userspace_access) {
    uint32_t* entry = (uint32_t*)page_table_address + entry_index;
    // printk("ROOT ADDR: %p\n, ", page_table_address);
    // printk("ENTRY: %p\n, ", *entry);
    *entry |= (1 << PTE_VALID_BIT);
    *entry |= (is_leaf << PTE_READ_BIT);
    *entry |= (is_leaf << PTE_WRITE_BIT);
    *entry |= (is_leaf << PTE_EXECUTE_BIT);
    *entry |= (userspace_access << PTE_USERMODE_BIT);
    // *entry |= (1 << PTE_DIRTY_BIT);
    // *entry |= (1 << PTE_ACCESSED_BIT);
    *entry |= (phys_addr >> 2);
}

/**
 *  Adds an entry to the first (root) level page table, and sets it to valid
 *
 *  @param as Address space to use.
 *  @param virtual_addr Virtual address to map in the entry
 *  @param phys_addr Physical address the entry should point to
 *  @param is_leaf Whether the entry should be a leaf entry
 *  @param userspace_access Whether the page will be accsessible from user mode
 */
static void _add_lvl1_pte(as_t* as, uintptr_t virtual_addr, uintptr_t phys_addr, bool is_leaf, bool userspace_access) {
    _add_pte(as->root_table_addr, _lvl1_entry_index(virtual_addr), phys_addr, is_leaf, userspace_access);
}

/**
 *  Adds an entry to the second level page table, and sets it to valid.
 *  If an entry pointing to the second level table doesn't exist in the root table,
 *  it is created as well.
 *
 *  @param as Address space to use.
 *  @param virtual_addr Virtual address to map in the entry
 *  @param phys_addr Physical address the entry should point to
 *  @param userspace_access Whether the page will be accsessible from user mode
 *  @retval EOK Operation successful
 *  @retval ENOMEM Out of memory
 */
static errno_t _add_lvl2_pte(as_t* as, uintptr_t virtual_addr, uintptr_t phys_addr, bool userspace_access) {
    uint32_t lvl1_entry = ((uint32_t*)as->root_table_addr)[_lvl1_entry_index(virtual_addr)];
    bool has_lvl2_table = lvl1_entry & PTE_VALIDITY_MASK;
    uintptr_t lvl2_table_address;

    if (!has_lvl2_table) {
        errno_t err = frame_alloc(1, &lvl2_table_address);
        if (err == ENOMEM) {
            return err;
        }

        _add_lvl1_pte(as, virtual_addr, lvl2_table_address, false, userspace_access);
        _table_init((uint32_t*)lvl2_table_address);
    } else {
        lvl2_table_address = (lvl1_entry & PTE_PPN_MASK) << 2;
    }

    _add_pte(lvl2_table_address, _lvl2_entry_index(virtual_addr), phys_addr, true, userspace_access);

    return EOK;
}

/**
 * Identity maps specified addresses without frame allocation
 * by megapages
 * @param as Address space for which we are creating mapping
 * @param start_addr First address to map
 * @param end_addr Last address to map
 */
static void _identity_map(as_t* as, uintptr_t start_addr, uintptr_t end_addr) {
    for (uintptr_t addr = start_addr; addr <= end_addr; addr += MEGAPAGE_SIZE) {
        _add_lvl1_pte(as, addr, addr, true, false);
    }
}

static void _map_kernel(as_t* as) {
    _identity_map(as, KERNEL_ADDRESS, get_memory_end());
}

void map_image(as_t* as, uintptr_t image_start, uintptr_t image_end) {
    _identity_map(as, image_start, image_end);
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
    address_space->asid = 0;
    address_space->using_threads = 0;

    _table_init((uint32_t*)address_space->root_table_addr);
    _map_kernel(address_space);
    _add_lvl1_pte(address_space, PRINTER_ADDRESS, PRINTER_ADDRESS, true, false);
    _add_lvl1_pte(address_space, LOADER_CODE_ADDRESS, LOADER_CODE_ADDRESS, true, false);

    return address_space;
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
 * @retval ENOMEM Out of memory
 */
uintptr_t as_alloc_page(as_t* as, uintptr_t virtual_addr, size_t count) {
    uintptr_t frame_phys_address;

    errno_t frame_alloc_err = frame_alloc(count, &frame_phys_address);
    if (frame_alloc_err == ENOMEM) {
        return ENOMEM;
    }

    for (size_t i = 0; i < count; ++i) {

        uintptr_t entry_virt_addr = virtual_addr + (i * PAGE_SIZE);
        uintptr_t entry_phys_addr = frame_phys_address + (i * FRAME_SIZE);

        errno_t pte_add_err = _add_lvl2_pte(as, entry_virt_addr, entry_phys_addr, true);

        if (pte_add_err == ENOMEM) {
            frame_free(count, frame_phys_address);
            return ENOMEM;
        }
    }

    return frame_phys_address;
}

/**
 * Iterates through the table and deallocates frame for every valid page
 * @param table_address Address of the table with entries to deallocate
 *
 */
static void _deallocate_entries(uint32_t* table_address) {
    for (size_t i = 0; i < PAGES_PER_TABLE; ++i) {
        uint32_t lvl2_entry = table_address[i];
        bool entry_is_valid = lvl2_entry & PTE_VALIDITY_MASK;

        if (entry_is_valid) {
            uintptr_t phys_addr = (lvl2_entry & PTE_PPN_MASK) << 2;
            frame_free(1, phys_addr);
        }
    }
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
    // printk("DESTROY\n");
    size_t userspace_entries_count = KERNEL_ADDRESS / MEGAPAGE_SIZE;
    for (size_t i = 0; i < userspace_entries_count; ++i) {
        uint32_t lvl1_entry = ((uint32_t*)as->root_table_addr)[i];
        bool entry_is_valid = lvl1_entry & PTE_VALIDITY_MASK;
        if (entry_is_valid) {
            uintptr_t lvl2_table_phys_addr = (lvl1_entry & PTE_PPN_MASK) << 2;
            // printk("ADDR: %x\n", lvl2_table_phys_addr);
            _deallocate_entries((uint32_t*)lvl2_table_phys_addr);
            frame_free(1, lvl2_table_phys_addr);
        }
    }

    // frame_free(1, as->root_table_addr);
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
static void _unsafe_as_add_thread(as_t* as, thread_t* thread) {
    thread->address_space = as;
    as->using_threads++;
}

void as_add_thread(as_t* as, thread_t* thread) {
    bool enabled = interrupts_disable();
    _unsafe_as_add_thread(as, thread);
    interrupts_restore(enabled);
}

/**
 * Removes address space from a thread, and decreses the spaces threads counter.
 * If there are no threads using the space left, the space is destroyed
 *
 * @param as Address space to assign
 * @param thread Thread to assign the space to
 */
static void _unsafe_as_remove_thread(thread_t* thread) {
    as_t* as = thread->address_space;
    // printk("USING: %d\n", as->using_threads);
    assert(as->using_threads != 0);

    thread->address_space = NULL;
    as->using_threads--;

    if (as->using_threads == 0) {
        as_destroy(as);
    }
}

void as_remove_thread(thread_t* thread) {
    bool enabled = interrupts_disable();
    _unsafe_as_remove_thread(thread);
    interrupts_restore(enabled);
}

bool is_illegal_memory_access(uintptr_t address, as_t* address_space) {
    return address_space == NULL
            || is_kernel_pages_access(address)
            || is_null_pages_access(address)
            || is_printer_pages_access(address)
            || is_address_beyond_address_space(address, address_space);
}