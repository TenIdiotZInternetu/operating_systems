// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <mm/as.h>

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
    return NULL;
}

/** Get the size (in bytes) of a given address space. */
size_t as_get_size(as_t* as) {
    return 0;
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
}

/** Get physical address of root page table.
 *
 * The function should return complete physical address, i.e. not only PPN.
 *
 * @param as Address space to use.
 * @return Physical address of root page table.
 */
uintptr_t as_get_root_page_table(as_t* as) {
    return 0;
}
