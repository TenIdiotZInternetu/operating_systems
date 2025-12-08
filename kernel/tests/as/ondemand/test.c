// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Charles University

/*
 * Determines if kernel supports on-demand page mapping (i.e. allocate
 * page only on page access).
 *
 * We check that we can create a relatively big address space (needing more
 * memory than we normally give to the simulator, hence unable to be created
 * in a kernel that preallocates all memory).
 *
 * Next we will randomly access memory of this address space and wait until
 * the thread is killed. We will then check that we were able to access some
 * minimal amount of pages before the thread was killed.
 */

#include <adt/bitmap.h>
#include <ktest.h>
#include <mm/as.h>
#include <proc/thread.h>

// Amount of pages for each address space
#define AS_SIZE_PAGES 64

// Amount of threads (address spaces) that we create.
// WORKER_COUNT * AS_SIZE_PAGES must exceed total memory
#define WORKER_COUNT 8

// Minimum amount of pages we expect to allocate across workers
#define PAGES_AVAILABLE_MIN 64

// Record which pages we were able to access
static bitmap_t accessed_pages[WORKER_COUNT];
static uint8_t accessed_pages_storage[WORKER_COUNT][BITMAP_GET_STORAGE_SIZE(AS_SIZE_PAGES)];

static volatile bool start_workers = false;

// Started threads
static thread_t* threads[WORKER_COUNT];

static inline uint8_t get_value_for_address(uintptr_t addr) {
    return ((addr / PAGE_SIZE) ^ (addr & 0xff)) & 0xff;
}

static inline uintptr_t get_address_in_page(size_t page) {
    // Not real randomness but at least we are not hitting exactly the same
    // spot.
    return page * PAGE_SIZE + (29 + page % 37);
}

static void* as_worker(void* accessed_pages_ptr) {
    bitmap_t* accessed_pages = (bitmap_t*)accessed_pages_ptr;

    while (!start_workers) {
        thread_yield();
    }

    size_t page_index = 5;
    size_t page_visited_count = PAGE_NULL_COUNT;

    bitmap_clear_range(accessed_pages, 0, AS_SIZE_PAGES);
    bitmap_fill_range(accessed_pages, 0, PAGE_NULL_COUNT);

    while (page_visited_count < AS_SIZE_PAGES) {
        size_t next_index;
        size_t hint = (page_index + 19) % AS_SIZE_PAGES;
        errno_t err = bitmap_find_range_with_hint(accessed_pages, 1, false, hint, &next_index);
        ktest_assert_errno(err, "bitmap_find_range_with_hint should always succeed here");

        if (page_visited_count > PAGE_NULL_COUNT) {
            uintptr_t prev_addr = get_address_in_page(page_index);
            uint8_t* prev_addr_ptr = (uint8_t*)prev_addr;
            uint8_t expected = get_value_for_address(prev_addr);
            uint8_t actual = *prev_addr_ptr;
            ktest_assert(expected == actual, "mismatching value at 0x%x (expecting %d, got %d)",
                    prev_addr, expected, actual);
        }

        uintptr_t next_addr = get_address_in_page(next_index);
        uint8_t* next_addr_ptr = (uint8_t*)next_addr;
        *next_addr_ptr = get_value_for_address((uintptr_t)next_addr);

        bitmap_set(accessed_pages, next_index, true);
        page_index = next_index;
        page_visited_count++;
    }

    return NULL;
}

void kernel_test(void) {
    ktest_start("as/ondemand");

    for (size_t i = 0; i < WORKER_COUNT; i++) {
        bitmap_init(&accessed_pages[i], AS_SIZE_PAGES, accessed_pages_storage[i]);
    }

    for (size_t i = 0; i < WORKER_COUNT; i++) {
        errno_t err = thread_create_new_as(&threads[i], as_worker, &accessed_pages[i], 0, "worker", AS_SIZE_PAGES * PAGE_SIZE);
        ktest_assert_errno(err, "on-demand AS page allocation should succeed");
    }

    start_workers = true;
    thread_yield();

    size_t killed_count = 0;
    for (size_t i = 0; i < WORKER_COUNT; i++) {
        errno_t err = thread_join(threads[i], NULL);
        if (err == EOK) {
            continue;
        }
        ktest_assert(err == EKILLED, "as_worker(index %u) should have been killed, but thread_join returned %s", i, errno_as_str(err));
        killed_count++;
    }

    ktest_assert(killed_count > 0, "we never had enough memory for all workers");

    size_t total_allocated_pages = 0;
    for (size_t i = 0; i < WORKER_COUNT; i++) {
        for (size_t j = 0; j < AS_SIZE_PAGES; j++) {
            if (bitmap_is_set(&accessed_pages[i], j)) {
                total_allocated_pages++;
            }
        }
    }

    ktest_assert(total_allocated_pages > PAGES_AVAILABLE_MIN,
            "threads should have seen more memory than this (expected %u, seen %u pages)",
            PAGES_AVAILABLE_MIN, total_allocated_pages);

    ktest_passed();
}
