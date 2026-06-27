/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026  Jakub Fietko <fietkojakub@proton.me>
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

#include <tasks/loader.h>
#include <tasks/tasks.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/elf64.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <memory/memtools.h>
#include <int/int.h>

extern vmm_table_t *kernel_pml4_virt;
extern void enter_user_mode(uintptr_t entry_point /*, uintptr_t user_stack*/);

void load_bin(void *data, int pages, size_t size)
{
    vmm_table_t *procvmm = vmm_init();
    size_t bytes_left = size;
    uint8_t *src_ptr = (uint8_t *)data;

    for (int i = 0; i < pages; i++)
    {
        void *phys_frame = pmm_alloc_frame();
        uintptr_t virt_frame = PROGRAM_START + (i * PAGE_SIZE);
        vmm_map_page(procvmm, virt_frame, (uintptr_t)phys_frame, VMM_PRESENT | VMM_WRITE | VMM_USER);

        uint8_t *kernel_virt_frame = (uint8_t *)((uintptr_t)phys_frame + hhdm_offset);

        size_t to_copy = (bytes_left > PAGE_SIZE) ? PAGE_SIZE : bytes_left;
        if (to_copy > 0)
        {
            memcpy(kernel_virt_frame, src_ptr, to_copy);
            src_ptr += to_copy;
            bytes_left -= to_copy;
        }
    }

    uintptr_t heap_virtual_start = 0xFFFFFFFFD0000000;
    void *frame = pmm_alloc_frame();
    vmm_map_page(procvmm, heap_virtual_start, (uintptr_t)frame, VMM_PRESENT | VMM_WRITE | VMM_USER);

    uintptr_t rsp = (heap_virtual_start + PAGE_SIZE) & ~0xFULL;
    void *kstack_phys = pmm_alloc_frame();
    uintptr_t kstack_virt = (uintptr_t)kstack_phys + hhdm_offset + PAGE_SIZE;

    tss.rsp0 = kstack_virt;

    vmm_switch_pml4(procvmm);
    asm volatile("mov %0, %%rsi" ::"r"(rsp) : "memory");
    enter_user_mode(PROGRAM_START /*, rsp*/);
    /*void (*program)(void) = (void (*)(void))PROGRAM_START;
    program();*/
}

void load_elf(void *elf_data)
{
    ELF64_Ehdr *header = (ELF64_Ehdr *)elf_data;

    if (*(uint32_t *)header->e_ident != ELF_MAGIC)
        kprintf("ELF Loader: invalid ELF magic\n");

    if (header->e_machine != 0x3E)
        kprintf("ELF Loader: not a x86_64 ELF\n");

    vmm_table_t *procvmm = vmm_init();

    ELF64_Phdr *ph_table = (ELF64_Phdr *)((uintptr_t)elf_data + header->e_phoff);

    for (int i = 0; i < header->e_phnum; i++)
    {
        ELF64_Phdr *phdr = &ph_table[i];

        if (phdr->p_type == PT_LOAD)
        {
            uintptr_t virt_start = phdr->p_vaddr;
            size_t mem_size = phdr->p_memsz;
            size_t file_size = phdr->p_filesz;

            uintptr_t page_offset = 0;
            while (page_offset < mem_size)
            {
                uintptr_t current_virt = (virt_start + page_offset) & ~0xFFFULL;
                void *phys_frame = pmm_alloc_frame();
                vmm_map_page(procvmm, current_virt, (uintptr_t)phys_frame, VMM_PRESENT | VMM_WRITE | VMM_USER);

                uint8_t *kernel_virt_frame = (uint8_t *)((uintptr_t)phys_frame + hhdm_offset);
                memset(kernel_virt_frame, 0, PAGE_SIZE);

                if (page_offset < file_size)
                {
                    size_t to_copy = file_size - page_offset;
                    if (to_copy > PAGE_SIZE)
                        to_copy = PAGE_SIZE;

                    memcpy(kernel_virt_frame, (uint8_t *)elf_data + phdr->p_offset + page_offset, to_copy);
                }

                page_offset += PAGE_SIZE;
            }
        }
    }

    uintptr_t heap_virtual_start = 0xFFFFFFFFD0000000;
    void *frame = pmm_alloc_frame();
    vmm_map_page(procvmm, heap_virtual_start, (uintptr_t)frame, VMM_PRESENT | VMM_WRITE | VMM_USER);
    uintptr_t rsp = (heap_virtual_start + PAGE_SIZE) & ~0xFULL;
    void *kstack_phys = pmm_alloc_frame();
    uintptr_t kstack_virt = (uintptr_t)kstack_phys + hhdm_offset + PAGE_SIZE;
    tss.rsp0 = kstack_virt;

    vmm_switch_pml4(procvmm);
    asm volatile("mov %0, %%rsi" ::"r"(rsp) : "memory");
    enter_user_mode(header->e_entry /*, rsp*/);
}
