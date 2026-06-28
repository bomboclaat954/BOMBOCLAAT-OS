/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef VMM_H
#define VMM_H
#include <stdint.h>
#include <boot/limine.h>

typedef uint64_t pt_entry_t;
typedef struct vmm_table
{
    pt_entry_t entries[512];
} __attribute__((aligned(4096))) vmm_table_t;

#define PROGRAM_START 0x400000
#define VMM_PRESENT (1ULL << 0)
#define VMM_WRITE (1ULL << 1)
#define VMM_USER (1ULL << 2)
#define VMM_PWT (1ULL << 3)
#define VMM_PCD (1ULL << 4)
#define VMM_NX (1ULL << 63)
#define CLEAR_FLAGS 0x000FFFFFFFFFF000ULL
#define PML4_INDEX(x) (((x) >> 39) & 0x1FF)
#define PDPT_INDEX(x) (((x) >> 30) & 0x1FF)
#define PD_INDEX(x) (((x) >> 21) & 0x1FF)
#define PT_INDEX(x) (((x) >> 12) & 0x1FF)

void vmm_map_page(vmm_table_t *pml4_virtual, uintptr_t virt, uintptr_t phys, uintptr_t flags);
void vmm_unmap_page(vmm_table_t *pml4_virtual, uintptr_t virt);
vmm_table_t *vmm_get_current_pml4(void);
vmm_table_t *vmm_init();
void vmm_switch_pml4(vmm_table_t *pml4);

#endif
