/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GLOBALS_H
#define GLOBALS_H
#include <int/int.h>
#include <memory/stack.h>
#include <boot/limine.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define NULL ((void *)0)
#define HEAP_SIZE 16 * (1024 * 1024) // 16 MB
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
    extern char *UNAME[3];
    extern stack_t system_stack;
    extern uint64_t hhdm_offset;

#ifdef __cplusplus
}
#endif

#endif
