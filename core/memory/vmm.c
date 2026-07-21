/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <memory/vmm.h>
#include <memory/pmm.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>

extern uint64_t hhdm_offset;

void vmm_map_page(vmm_table_t *pml4_virtual, uintptr_t virt, uintptr_t phys, uintptr_t flags)
{
    uintptr_t perm_flags = flags & 0xFFF;

    if (!(pml4_virtual->entries[PML4_INDEX(virt)] & VMM_PRESENT))
    {
        uintptr_t new_table_phys = (uintptr_t)pmm_alloc_frame();
        if (new_table_phys == 0)
            panic("vmm_map_page: pmm_alloc_frame failed", 0, 0);
        vmm_table_t *new_table_virt = (vmm_table_t *)(new_table_phys + hhdm_offset);
        memset(new_table_virt, 0, sizeof(vmm_table_t));
        pml4_virtual->entries[PML4_INDEX(virt)] = new_table_phys | perm_flags;
    }
    else
        pml4_virtual->entries[PML4_INDEX(virt)] |= perm_flags;

    uintptr_t pdpt_phys = pml4_virtual->entries[PML4_INDEX(virt)] & CLEAR_FLAGS;
    vmm_table_t *pdpt_virtual = (vmm_table_t *)(pdpt_phys + hhdm_offset);

    if (!(pdpt_virtual->entries[PDPT_INDEX(virt)] & VMM_PRESENT))
    {
        uintptr_t new_table_phys = (uintptr_t)pmm_alloc_frame();
        if (new_table_phys == 0)
            panic("vmm_map_page: pmm_alloc_frame failed", 0, 0);
        vmm_table_t *new_table_virt = (vmm_table_t *)(new_table_phys + hhdm_offset);
        memset(new_table_virt, 0, sizeof(vmm_table_t));
        pdpt_virtual->entries[PDPT_INDEX(virt)] = new_table_phys | perm_flags;
    }
    else
        pdpt_virtual->entries[PDPT_INDEX(virt)] |= perm_flags;

    uintptr_t pd_phys = pdpt_virtual->entries[PDPT_INDEX(virt)] & CLEAR_FLAGS;
    vmm_table_t *pd_virtual = (vmm_table_t *)(pd_phys + hhdm_offset);

    if (!(pd_virtual->entries[PD_INDEX(virt)] & VMM_PRESENT))
    {
        uintptr_t new_table_phys = (uintptr_t)pmm_alloc_frame();
        if (new_table_phys == 0)
            panic("vmm_map_page: pmm_alloc_frame failed", 0, 0);
        vmm_table_t *new_table_virt = (vmm_table_t *)(new_table_phys + hhdm_offset);
        memset(new_table_virt, 0, sizeof(vmm_table_t));
        pd_virtual->entries[PD_INDEX(virt)] = new_table_phys | perm_flags;
    }
    else
        pd_virtual->entries[PD_INDEX(virt)] |= perm_flags;

    uintptr_t pt_phys = pd_virtual->entries[PD_INDEX(virt)] & CLEAR_FLAGS;
    vmm_table_t *pt_virtual = (vmm_table_t *)(pt_phys + hhdm_offset);
    pt_virtual->entries[PT_INDEX(virt)] = phys | flags | VMM_PRESENT;

    asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void vmm_unmap_page(vmm_table_t *pml4_virtual, uintptr_t virt)
{
    pt_entry_t pml4_entry = pml4_virtual->entries[PML4_INDEX(virt)];
    if (!(pml4_entry & VMM_PRESENT))
        return;
    uintptr_t pdpt_phys = pml4_entry & CLEAR_FLAGS;
    vmm_table_t *pdpt_virtual = (vmm_table_t *)(pdpt_phys + hhdm_offset);
    pt_entry_t pdpt_entry = pdpt_virtual->entries[PDPT_INDEX(virt)];
    if (!(pdpt_entry & VMM_PRESENT))
        return;
    uintptr_t pd_phys = pdpt_entry & CLEAR_FLAGS;
    vmm_table_t *pd_virtual = (vmm_table_t *)(pd_phys + hhdm_offset);
    pt_entry_t pd_entry = pd_virtual->entries[PD_INDEX(virt)];
    if (!(pd_entry & VMM_PRESENT))
        return;
    uintptr_t pt_phys = pd_entry & CLEAR_FLAGS;
    vmm_table_t *pt_virtual = (vmm_table_t *)(pt_phys + hhdm_offset);
    pt_virtual->entries[PT_INDEX(virt)] = 0;
    asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

vmm_table_t *vmm_get_current_pml4(void)
{
    uintptr_t pml4_phys;
    asm volatile("mov %%cr3, %0" : "=r"(pml4_phys));
    return (vmm_table_t *)(pml4_phys + hhdm_offset);
}

vmm_table_t *vmm_init()
{
    extern vmm_table_t *kernel_pml4_virt;
    uintptr_t new_pml4_phys = (uintptr_t)pmm_alloc_frame();
    if (new_pml4_phys == 0)
        return NULL;

    vmm_table_t *table = (vmm_table_t *)(new_pml4_phys + hhdm_offset);

    for (int i = 0; i < 256; i++)
        table->entries[i] = 0;

    for (int i = 256; i < 512; i++)
        table->entries[i] = kernel_pml4_virt->entries[i];

    return table;
}

vmm_table_t *vmm_init_kernel()
{
    uintptr_t new_pml4_phys = (uintptr_t)pmm_alloc_frame();
    if (new_pml4_phys == 0)
        return NULL;

    vmm_table_t *table = (vmm_table_t *)(new_pml4_phys + hhdm_offset);

    for (int i = 0; i < 256; i++)
        table->entries[i] = 0;

    uintptr_t old_pml4_phys;
    asm volatile("mov %%cr3, %0" : "=r"(old_pml4_phys));
    vmm_table_t *old_pml4_virt = (vmm_table_t *)(old_pml4_phys + hhdm_offset);

    for (int i = 256; i < 512; i++)
        table->entries[i] = old_pml4_virt->entries[i];

    return table;
}

void vmm_switch_pml4(vmm_table_t *pml4)
{
    uintptr_t pml4_phys = (uintptr_t)pml4 - hhdm_offset;
    asm volatile("mov %0, %%cr3" ::"r"(pml4_phys) : "memory");
}
