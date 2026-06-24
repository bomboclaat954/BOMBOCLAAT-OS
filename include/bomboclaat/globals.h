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
    extern char *UNAME[4];
    extern stack_t system_stack;
    extern uint64_t hhdm_offset;

    typedef struct
    {
        int h_shift;
        int m_shift;
        int fat32;
        uint32_t current_dir_cluster;
        uint32_t root_cluster;
        char current_path[128];
    } global_settings;

    extern global_settings settings;

#ifdef __cplusplus
}
#endif

#endif
