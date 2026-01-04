// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <stdlib.h>

/** Allocate size bytes on heap.
 *
 * When size is zero, the behavior is implementation-defined with two options:
 * return NULL or return a unique pointer. In both of these cases, the returned
 * value can be later succesfully passed to free. Consult the C standard for
 * more details.
 *
 * @param size Amount of memory to allocate.
 * @return Pointer to allocated memory.
 * @retval NULL Out of memory.
 */
void* malloc(size_t size) {
    return NULL;
}

/** Free memory previously allocated by malloc.
 *
 * The ptr parameter must be either a pointer previously returned by a call
 * to malloc or NULL (in that case, no action is performed).
 *
 * @param ptr Pointer to memory to be freed.
 */
void free(void* ptr) {
}
