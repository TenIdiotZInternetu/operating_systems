// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Charles University

#ifndef _ADT_BITS_H
#define _ADT_BITS_H

#include <types.h>

/** Stores a value into a bitfield.
 *
 * The input value is masked to limit its width and the result is shifted
 * left to the position of the bitfield in the output word.
 */
#define bitfield_put(value, width, shift) \
    (((value) & ((1 << (width)) - 1)) << (shift))

/** Extracts a value from a bitfield.
 *
 * The input bitfield value is shifted right and masked to only extract
 * the bits belonging to the bitfield.
 */
#define bitfield_get(value, width, shift) \
    ((((unsigned)(value)) >> shift) & ((1 << (width)) - 1))

/** Calculates the mask for a given bitfield.
 *
 * Because we don't have an input value, the resulting type
 * of the mask is the same as the type of the width.
 */
#define bitfield_mask(width, shift) \
    (((((__typeof__(width))1) << (width)) - 1) << (shift))

static inline bool is_power_of_two(unsigned long x) {
    /*
     * Using a trick here: if x is power of 2, then
     * it has no common bits with (x-1) [one bit set
     * vs. all lower bits set].
     *
     * And we consider 0 as a special case.
     */
    return (x != 0) && !(x & (x - 1));
}

#endif
