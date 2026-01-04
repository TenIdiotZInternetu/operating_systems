// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Dummy section to provide space for stack and NULL page.
 */

.section .pre_entry_space, "ax"

.global __pre_entry_space

.org 0x0
__pre_entry_space:
    nop
.end __pre_entry_space
