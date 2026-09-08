/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GLOBALS_H
#define GLOBALS_H
#include <memory/stack.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define NULL ((void *)0)
#define HEAP_SIZE 32 * (1024 * 1024) // 32 MB
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SHIFT_L(x, y) x << y
#define SHIFT_R(x, y) x >> y
    extern stack_t system_stack;
    extern uint64_t hhdm_offset;

#ifdef __cplusplus
}
#endif

#endif
