/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PMM_H
#define PMM_H
#include <stdint.h>
#include <stddef.h>
#include <boot/limine.h>

#define PAGE_SIZE 4096

struct ram // a lil bit useless but works
{
    int total;
    int usable;
} typedef ram_t;

uintptr_t get_total_frames();
uintptr_t get_free_frames();
void *pmm_alloc_frame();
void pmm_free_frame(void *phys);
void pmm_init(struct limine_memmap_response *memmap, struct limine_hhdm_response *hhdm);
ram_t init_memmap(struct limine_memmap_response *memmap);

#endif
