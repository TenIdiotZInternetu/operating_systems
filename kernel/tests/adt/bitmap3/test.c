// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/bitmap.h>
#include <ktest.h>

/*
 * Bitmap ranges test.
 */

#define SIZE 256

#define assert_find_range(bitmap, count, bits_set, expected_result, assertion_message) \
    do { \
        size_t start_; \
        errno_t err = bitmap_find_range(bitmap, count, bits_set, &start_); \
        ktest_assert_errno(err, "bitmap_find_range"); \
        ktest_assert(start_ == (expected_result), \
                "Expecting start at %u, got %u (%s)", \
                expected_result, start_, assertion_message); \
    } while (false)

#define assert_find_range_with_hint(bitmap, count, bits_set, hint, expected_result, assertion_message) \
    do { \
        size_t start_; \
        errno_t err = bitmap_find_range_with_hint(bitmap, count, bits_set, hint, &start_); \
        ktest_assert_errno(err, "bitmap_find_range"); \
        ktest_assert(start_ == (expected_result), \
                "Expecting start at %u (hint %u), got %u (%s)", \
                expected_result, hint, start_, assertion_message); \
    } while (false)

void kernel_test(void) {
    ktest_start("adt/bitmap3");

    uint8_t storage[BITMAP_GET_STORAGE_SIZE(SIZE)];
    bitmap_t bm;
    bitmap_init(&bm, SIZE, storage);

    assert_find_range(&bm, 4, false, 0, "whole bitmap is free");
    assert_find_range_with_hint(&bm, 4, false, 0, 0, "whole bitmap is free and hint points to 0");
    assert_find_range_with_hint(&bm, 127, false, 127, 127, "whole bitmap is free but hint was set to 127");

    bitmap_fill_range(&bm, 59, 7);

    assert_find_range(&bm, 72, false, 66, "has to skip full block");
    assert_find_range_with_hint(&bm, 72, false, 0, 66, "has to skip full block");
    assert_find_range_with_hint(&bm, 72, false, 60, 66, "has to skip full block");
    assert_find_range_with_hint(&bm, 72, false, 66, 66, "has to skip full block");
    assert_find_range_with_hint(&bm, 72, false, 240, 66, "has to skip full block");

    assert_find_range(&bm, 47, false, 0, "has to fit before full block");
    assert_find_range_with_hint(&bm, 47, false, 0, 0, "has to fit before full block with hint 0");
    assert_find_range_with_hint(&bm, 47, false, 47, 66, "found after hint");
    assert_find_range_with_hint(&bm, 47, false, 62, 66, "found after hint");
    assert_find_range_with_hint(&bm, 47, false, 240, 0, "wrapped hinted search");

    ktest_passed();
}
