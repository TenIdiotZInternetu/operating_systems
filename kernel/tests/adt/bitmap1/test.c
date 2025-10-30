// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/bitmap.h>
#include <ktest.h>

/*
 * Basic bitmap test. The constants are intentionally too big.
 *
 * We only test that setting bits at bytes boundary do not corrupt surrounding
 * bits.
 */

static void memset(uint8_t* buf, size_t size, uint8_t val) {
    for (size_t i = 0; i < size; i++) {
        buf[i] = val;
    }
}

static void check_bitmap_init(bitmap_t* bm, size_t length, uint8_t* storage, size_t storage_size) {
    size_t required_size = BITMAP_GET_STORAGE_SIZE(length);
    assert(required_size < storage_size);

    memset(storage, storage_size, 0xaa);
    bitmap_init(bm, length, storage);

    size_t first_untouched = required_size + 1;
    ktest_assert(storage[first_untouched] == 0xaa, "initialization does not corrupt memory");

    for (size_t i = 0; i < length; i++) {
        ktest_assert(!bitmap_is_set(bm, i), "initialization resets all to 0 (%u)", i);
    }
}

#define LENGTH ((BITMAP_BITS_PER_UNIT * 2) + 1)

void kernel_test(void) {
    ktest_start("adt/bitmap1");

    uint8_t storage[2 * BITMAP_GET_STORAGE_SIZE(LENGTH)];

    bitmap_t bm;
    check_bitmap_init(&bm, BITMAP_BITS_PER_UNIT / 2, storage, sizeof(storage));
    check_bitmap_init(&bm, BITMAP_BITS_PER_UNIT - 1, storage, sizeof(storage));
    check_bitmap_init(&bm, BITMAP_BITS_PER_UNIT, storage, sizeof(storage));
    check_bitmap_init(&bm, BITMAP_BITS_PER_UNIT + 1, storage, sizeof(storage));
    check_bitmap_init(&bm, BITMAP_BITS_PER_UNIT * 2, storage, sizeof(storage));

    // The test configuration.
    check_bitmap_init(&bm, LENGTH, storage, sizeof(storage));

    bitmap_set(&bm, 6, true);
    bitmap_set(&bm, 7, true);
    bitmap_set(&bm, 8, true);
    bitmap_set(&bm, 9, true);

    ktest_assert(!bitmap_is_set(&bm, 0), "index 0 corrupted");
    ktest_assert(!bitmap_is_set(&bm, 1), "index 1 corrupted");
    ktest_assert(!bitmap_is_set(&bm, 2), "index 2 corrupted");
    ktest_assert(!bitmap_is_set(&bm, 3), "index 3 corrupted");
    ktest_assert(!bitmap_is_set(&bm, 4), "index 4 corrupted");
    ktest_assert(!bitmap_is_set(&bm, 5), "index 5 corrupted");
    ktest_assert(!bitmap_is_set(&bm, 10), "index 10 corrupted");
    ktest_assert(!bitmap_is_set(&bm, 11), "index 11 corrupted");

    ktest_assert(bitmap_is_set(&bm, 6), "index 6 not set");
    ktest_assert(bitmap_is_set(&bm, 7), "index 7 not set");
    ktest_assert(bitmap_is_set(&bm, 8), "index 8 not set");
    ktest_assert(bitmap_is_set(&bm, 9), "index 9 not set");

    ktest_passed();
}
