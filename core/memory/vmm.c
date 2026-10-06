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

int vmm_map_page(vmm_table_t *pml4_virtual, uintptr_t virt, uintptr_t phys, uintptr_t flags)
{
    uintptr_t perm_flags = flags & 0xFFF;

    if (!(pml4_virtual->entries[PML4_INDEX(virt)] & VMM_PRESENT))
    {
        uintptr_t new_table_phys = (uintptr_t)pmm_alloc_frame();
        if (new_table_phys == 0)
            return 1;
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
            return 2;
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
            return 3;
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
    return 0;
}

int vmm_unmap_page(vmm_table_t *pml4_virtual, uintptr_t virt)
{
    pt_entry_t pml4_entry = pml4_virtual->entries[PML4_INDEX(virt)];
    if (!(pml4_entry & VMM_PRESENT))
        return -1;
    uintptr_t pdpt_phys = pml4_entry & CLEAR_FLAGS;
    vmm_table_t *pdpt_virtual = (vmm_table_t *)(pdpt_phys + hhdm_offset);
    pt_entry_t pdpt_entry = pdpt_virtual->entries[PDPT_INDEX(virt)];
    if (!(pdpt_entry & VMM_PRESENT))
        return -1;
    uintptr_t pd_phys = pdpt_entry & CLEAR_FLAGS;
    vmm_table_t *pd_virtual = (vmm_table_t *)(pd_phys + hhdm_offset);
    pt_entry_t pd_entry = pd_virtual->entries[PD_INDEX(virt)];
    if (!(pd_entry & VMM_PRESENT))
        return -1;
    uintptr_t pt_phys = pd_entry & CLEAR_FLAGS;
    vmm_table_t *pt_virtual = (vmm_table_t *)(pt_phys + hhdm_offset);
    pt_virtual->entries[PT_INDEX(virt)] = 0;
    asm volatile("invlpg (%0)" ::"r"(virt) : "memory");

    return 0;
}

int vmm_remap_page(vmm_table_t *pml4_virtual, uintptr_t virt, uintptr_t phys, uintptr_t flags)
{
    if (vmm_unmap_page(pml4_virtual, virt) != 0)
        return -1;

    if (vmm_map_page(pml4_virtual, virt, phys, flags) != 0)
        return -1;

    return 0;
}

int vmm_resolve(vmm_table_t *pml4_virtual, uintptr_t virt, uintptr_t *phys_out, uint64_t *flags_out)
{
    if (!pml4_virtual || !phys_out || !flags_out)
        return -1;

    pt_entry_t entry = pml4_virtual->entries[PML4_INDEX(virt)];
    if (!(entry & VMM_PRESENT))
        return -1;
    uint64_t eff = entry & (VMM_WRITE | VMM_USER);

    vmm_table_t *pdpt_virtual = (vmm_table_t *)((entry & CLEAR_FLAGS) + hhdm_offset);
    entry = pdpt_virtual->entries[PDPT_INDEX(virt)];
    if (!(entry & VMM_PRESENT))
        return -1;
    eff &= entry | ~(uint64_t)(VMM_WRITE | VMM_USER);
    if (entry & (1ULL << 7))
    {
        *phys_out = (entry & 0x000FFFFFC0000000ULL) | (virt & 0x3FFFFFFFULL);
        *flags_out = (entry & 0xFFF & ~(uint64_t)(VMM_WRITE | VMM_USER)) | eff;
        return 0;
    }

    vmm_table_t *pd_virtual = (vmm_table_t *)((entry & CLEAR_FLAGS) + hhdm_offset);
    entry = pd_virtual->entries[PD_INDEX(virt)];
    if (!(entry & VMM_PRESENT))
        return -1;
    eff &= entry | ~(uint64_t)(VMM_WRITE | VMM_USER);
    if (entry & (1ULL << 7))
    {
        *phys_out = (entry & 0x000FFFFFFFE00000ULL) | (virt & 0x1FFFFFULL);
        *flags_out = (entry & 0xFFF & ~(uint64_t)(VMM_WRITE | VMM_USER)) | eff;
        return 0;
    }

    vmm_table_t *pt_virtual = (vmm_table_t *)((entry & CLEAR_FLAGS) + hhdm_offset);
    entry = pt_virtual->entries[PT_INDEX(virt)];
    if (!(entry & VMM_PRESENT))
        return -1;
    eff &= entry | ~(uint64_t)(VMM_WRITE | VMM_USER);

    *phys_out = (entry & CLEAR_FLAGS) | (virt & 0xFFF);
    *flags_out = (entry & 0xFFF & ~(uint64_t)(VMM_WRITE | VMM_USER)) | eff;

    return 0;
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

extern int vmm_is_shared_frame(uintptr_t phys);

static void vmm_copy_frame(uintptr_t dst_phys, uintptr_t src_phys)
{
    uint64_t *dst = (uint64_t *)(dst_phys + hhdm_offset);
    uint64_t *src = (uint64_t *)(src_phys + hhdm_offset);
    for (int i = 0; i < PAGE_SIZE / 8; i++)
        dst[i] = src[i];
}

vmm_table_t *vmm_clone_user_space(vmm_table_t *parent_pml4)
{
    if (!parent_pml4)
        return NULL;

    vmm_table_t *child = vmm_init();
    if (!child)
        return NULL;

    for (uintptr_t i4 = 0; i4 < 256; i4++)
    {
        pt_entry_t e4 = parent_pml4->entries[i4];
        if (!(e4 & VMM_PRESENT))
            continue;
        vmm_table_t *pdpt = (vmm_table_t *)((e4 & CLEAR_FLAGS) + hhdm_offset);

        for (uintptr_t i3 = 0; i3 < 512; i3++)
        {
            pt_entry_t e3 = pdpt->entries[i3];
            if (!(e3 & VMM_PRESENT))
                continue;
            vmm_table_t *pd = (vmm_table_t *)((e3 & CLEAR_FLAGS) + hhdm_offset);

            for (uintptr_t i2 = 0; i2 < 512; i2++)
            {
                pt_entry_t e2 = pd->entries[i2];
                if (!(e2 & VMM_PRESENT))
                    continue;
                vmm_table_t *pt = (vmm_table_t *)((e2 & CLEAR_FLAGS) + hhdm_offset);

                for (uintptr_t i1 = 0; i1 < 512; i1++)
                {
                    pt_entry_t e1 = pt->entries[i1];
                    if (!(e1 & VMM_PRESENT))
                        continue;

                    uintptr_t virt = (i4 << 39) | (i3 << 30) | (i2 << 21) | (i1 << 12);
                    uintptr_t phys = e1 & CLEAR_FLAGS;
                    uintptr_t flags = e1 & (0xFFFULL | VMM_NX);

                    if (vmm_is_shared_frame(phys))
                    {
                        if (vmm_map_page(child, virt, phys, flags) != 0)
                            goto fail;
                        continue;
                    }

                    uintptr_t copy = (uintptr_t)pmm_alloc_frame();
                    if (!copy)
                        goto fail;
                    vmm_copy_frame(copy, phys);
                    if (vmm_map_page(child, virt, copy, flags) != 0)
                    {
                        pmm_free_frame((void *)copy);
                        goto fail;
                    }
                }
            }
        }
    }

    return child;

fail:
    vmm_free(child);
    return NULL;
}

void vmm_free(vmm_table_t *pml4)
{
    extern vmm_table_t *kernel_pml4_virt;
    if (!pml4 || pml4 == kernel_pml4_virt)
        return;

    for (uintptr_t i4 = 0; i4 < 256; i4++)
    {
        pt_entry_t e4 = pml4->entries[i4];
        if (!(e4 & VMM_PRESENT))
            continue;
        vmm_table_t *pdpt = (vmm_table_t *)((e4 & CLEAR_FLAGS) + hhdm_offset);

        for (uintptr_t i3 = 0; i3 < 512; i3++)
        {
            pt_entry_t e3 = pdpt->entries[i3];
            if (!(e3 & VMM_PRESENT))
                continue;
            vmm_table_t *pd = (vmm_table_t *)((e3 & CLEAR_FLAGS) + hhdm_offset);

            for (uintptr_t i2 = 0; i2 < 512; i2++)
            {
                pt_entry_t e2 = pd->entries[i2];
                if (!(e2 & VMM_PRESENT))
                    continue;
                vmm_table_t *pt = (vmm_table_t *)((e2 & CLEAR_FLAGS) + hhdm_offset);

                for (uintptr_t i1 = 0; i1 < 512; i1++)
                {
                    pt_entry_t e1 = pt->entries[i1];
                    if (!(e1 & VMM_PRESENT))
                        continue;
                    uintptr_t phys = e1 & CLEAR_FLAGS;
                    if (!vmm_is_shared_frame(phys))
                        pmm_free_frame((void *)phys);
                }
                pmm_free_frame((void *)(e2 & CLEAR_FLAGS));
            }
            pmm_free_frame((void *)(e3 & CLEAR_FLAGS));
        }
        pmm_free_frame((void *)(e4 & CLEAR_FLAGS));
    }

    pmm_free_frame((void *)((uintptr_t)pml4 - hhdm_offset));
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
