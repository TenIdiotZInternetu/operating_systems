// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/proc.h>
#include <np/syscall.h>

/** Get information about current process.
 *
 * Note that id can be simply address of the kernel structure, the only
 * requirement is that it shall not change between calls.
 *
 * virt_mem_size shall be the total amount of virtual memory this process
 * has available (i.e. first invalid address is virt_mem_size + 2*4096).
 *
 * total_ticks shall represent accounting for the process, however we will
 * accept a solution that only increments the counter with every call to
 * this function.
 *
 * @param info Where to store the retrieved information.
 * @return true if the call was successful, false otherwise.
 */
bool np_proc_info_get(np_proc_info_t* info) {
    return false;
}
