// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <errno.h>
#include <mm/heap.h>
#include <proc/thread.h>

#define IMAGE_NULL_PADDING (PAGE_SIZE * 2)

void process_copy_image(process_t* process) {
    // The process is image is padded so code begins at the same offset as the address in address space
    for (size_t i = 0; i < process->image_size; ++i) {
        size_t entry_addr = PROCESS_ENTRY_POINT + i;

        if (entry_addr % PAGE_SIZE == 0) {
            as_t* as = process->user_thread->address_space;
            as_alloc_page(as, entry_addr, 1);
        }

        uint8_t* address_space = (uint8_t*)0;
        uint8_t* process_image = (uint8_t*)process->image_start;

        address_space[entry_addr] = process_image[entry_addr];
        printk("%x %x %x %x\n", process->image_start, entry_addr, address_space[entry_addr], process_image[entry_addr]);
    }
}

/** Create new userspace process.
 *
 * @param process_out Where to place the initialized process_t structure.
 * @param image_location Virtual address (in kernel segment) where is the image of the raw application binary.
 * @param image_size Size of the application binary image.
 * @param process_memory_size Amount of virtual memory to give to the application (at least image_size).
 * @return Error code.
 * @retval EOK Process was created and its main thread started.
 * @retval ENOMEM Not enough memory to complete the operation.
 * @retval EINVAL Invalid call (unaligned size etc.).
 */
static errno_t _unsafe_process_create(process_t** process_out, uintptr_t image_location, size_t image_size, size_t process_memory_size) {
    if (process_memory_size % PAGE_SIZE != 0) {
        return EINVAL;
    }

    thread_t* process_thread;
    errno_t err = thread_create_new_as(&process_thread, NULL, NULL, THREAD_USERSPACE_FLAG, "process_thread", process_memory_size);
    if (err != EOK) {
        return err;
    }

    process_t* process = kmalloc(sizeof(process_t));
    if (process == NULL) {
        return ENOMEM;
    }

    process->user_thread = process_thread;
    process->image_start = image_location;
    process->image_size = image_size;
    process_thread->process = process;

    *process_out = process;
    return EOK;
}

errno_t process_create(process_t** process_out, uintptr_t image_location, size_t image_size, size_t process_memory_size) {
    bool enabled = interrupts_disable();
    errno_t err = _unsafe_process_create(process_out, image_location, image_size, process_memory_size);
    interrupts_restore(enabled);
    return err;
}

/** Wait for termination of another process.
 *
 * @param process Process to wait for.
 * @param exit_status Where to place the process exit status (return value from main).
 * @return Error code.
 * @retval EOK Joined successfully.
 * @retval EBUSY Some other thread is already joining this process.
 * @retval EKILLED Process was killed.
 * @retval EINVAL Invalid process.
 */
errno_t process_join(process_t* process, int* exit_status) {
    if (process == NULL) {
        return EINVAL;
    }

    errno_t err = thread_join(process->user_thread, NULL);
    if (err != EOK) {
        return err;
    }

    *exit_status = process->exit_status;
    kfree(process);
    return EOK;
}
