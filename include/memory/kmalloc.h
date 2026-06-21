/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void heap_init(void *start, size_t size);
    void *kmalloc(size_t size);
    void kfree(void *ptr);
    void *kmalloc_aligned(size_t size, size_t alignment);

#ifdef __cplusplus
}
#endif

#endif
