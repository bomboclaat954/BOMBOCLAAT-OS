/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct block_header
{
    size_t size;
    bool is_free;
    struct block_header *next;
} block_header_t;
#define HEADER_SIZE sizeof(block_header_t)

void *sbrk(size_t increment);
void *malloc(size_t size);
void free(void *ptr);
