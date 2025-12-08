// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_USERSPACE_H
#define _PROC_USERSPACE_H

void cpu_jump_to_userspace(uintptr_t stack_top, uintptr_t entry);

#endif
