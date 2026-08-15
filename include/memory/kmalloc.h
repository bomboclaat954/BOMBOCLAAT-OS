/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

typedef struct kmem_block
{
    size_t size;
    struct kmem_block *next;
    uint8_t free;

} kmem_block_t;

typedef struct
{
    size_t size;
} chunk_header_t;

#ifdef __cplusplus
extern "C"
{
#endif

    void heap_init(void *start, size_t size);
    void *kmalloc(size_t size);
    void kfree(void *ptr);
    void *kmalloc_aligned(size_t size, size_t alignment);
    void *realloc(void *ptr, size_t size);
    size_t kptrsize(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
