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

// It took me over a week to make it work

#include <tasks/tasks.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <int/int.h>
#include <bomboclaat/elf64.h>
#include <lib/string.h>

extern vmm_table_t *kernel_pml4_virt;
extern uint64_t hhdm_offset;
extern tss_ptr tss;

task_t *current_task = NULL;
task_t *task_list_head = NULL;
int next_pid = 1;

void task_init(void)
{
    task_t *kernel_task = (task_t *)kmalloc(sizeof(task_t));
    kernel_task->pid = 0;
    strcpy("kernel", kernel_task->name);
    kernel_task->state = TASK_BLOCKED;
    kernel_task->pml4 = kernel_pml4_virt;

    void *kstack_phys = pmm_alloc_frame();
    kernel_task->kstack_top = (uintptr_t)kstack_phys + hhdm_offset + PAGE_SIZE;
    kernel_task->rsp = kernel_task->kstack_top - sizeof(context_t);

    context_t *ctx = (context_t *)kernel_task->rsp;
    memset(ctx, 0, sizeof(context_t));

    kernel_task->next = kernel_task;
    kernel_task->parent_pid = 0;
    current_task = kernel_task;
    task_list_head = kernel_task;
}

task_t *task_create(void *elf_data, int parent_pid, char *name, int frames)
{
    ELF64_Ehdr *header = (ELF64_Ehdr *)elf_data;
    if (*(uint32_t *)header->e_ident != ELF_MAGIC || header->e_machine != 0x3E)
        return NULL;

    task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
    strcpy(name, new_task->name);
    new_task->pid = next_pid++;
    new_task->parent_pid = parent_pid;
    new_task->pml4 = vmm_init();
    ELF64_Phdr *ph_table = (ELF64_Phdr *)((uintptr_t)elf_data + header->e_phoff);
    for (int i = 0; i < header->e_phnum; i++)
    {
        ELF64_Phdr *phdr = &ph_table[i];
        if (phdr->p_type == PT_LOAD)
        {
            uintptr_t page_boundary_dist = phdr->p_vaddr & 0xFFF;

            uintptr_t current_virt = phdr->p_vaddr & ~0xFFFULL;
            size_t bytes_written = 0;
            size_t mem_size = phdr->p_memsz;
            size_t file_size = phdr->p_filesz;
            size_t file_bytes_written = 0;

            while (bytes_written < mem_size)
            {
                void *phys_frame = pmm_alloc_frame();
                vmm_map_page(new_task->pml4, current_virt, (uintptr_t)phys_frame, VMM_PRESENT | VMM_WRITE | VMM_USER);

                uint8_t *kvirt = (uint8_t *)((uintptr_t)phys_frame + hhdm_offset);
                memset(kvirt, 0, PAGE_SIZE);

                uintptr_t dest_off = (bytes_written == 0) ? page_boundary_dist : 0;
                size_t space = PAGE_SIZE - dest_off;

                if (file_bytes_written < file_size)
                {
                    size_t to_copy = file_size - file_bytes_written;
                    if (to_copy > space)
                        to_copy = space;

                    memcpy(kvirt + dest_off,
                           (uint8_t *)elf_data + phdr->p_offset + file_bytes_written,
                           to_copy);

                    file_bytes_written += to_copy;
                }

                bytes_written += space;
                current_virt += PAGE_SIZE;
            }
        }
    }

    uintptr_t user_stack_virtual = 0xFFFFFFFFD0000000;

    for (int i = 0; i < frames; i++)
    {
        void *frame_phys = pmm_alloc_frame();
        vmm_map_page(new_task->pml4, user_stack_virtual + (i * PAGE_SIZE), (uintptr_t)frame_phys, VMM_PRESENT | VMM_WRITE | VMM_USER);
    }

    uintptr_t user_rsp = (user_stack_virtual + (PAGE_SIZE * frames)) & ~0xFULL;

    void *kstack_phys = pmm_alloc_frame();
    new_task->kstack_top = (uintptr_t)kstack_phys + hhdm_offset + PAGE_SIZE;

    new_task->rsp = new_task->kstack_top - sizeof(context_t);
    context_t *ctx = (context_t *)new_task->rsp;
    memset(ctx, 0, sizeof(context_t));

    ctx->rip = header->e_entry;
    ctx->rsp = user_rsp;
    ctx->cs = 0x43;
    ctx->ss = 0x3B;
    ctx->rflags = 0x202;
    ctx->int_no = 0;
    ctx->err_code = 0;

    new_task->next = current_task->next;
    current_task->next = new_task;
    new_task->state = TASK_READY;

    return new_task;
}

context_t *schedule(context_t *ctx)
{
    if (current_task == NULL)
        return ctx;

    current_task->rsp = (uintptr_t)ctx;

    if (current_task->state == TASK_RUNNING)
        current_task->state = TASK_READY;

    task_t *next = current_task->next;
    int safety_counter = 0;

    while (next->state != TASK_READY && safety_counter < 64)
    {
        next = next->next;
        safety_counter++;
    }

    if (next->state != TASK_READY)
    {
        task_t *search = current_task;
        do
        {
            if (search->pid == 0)
            {
                next = search;
                break;
            }
            search = search->next;
        } while (search != current_task);
    }

    current_task = next;
    current_task->state = TASK_RUNNING;

    tss.rsp0 = current_task->kstack_top;
    uintptr_t next_cr3 = (uintptr_t)current_task->pml4 - hhdm_offset;
    // asm volatile("mov %0, %%cr3" : : "r"(next_cr3) : "memory");
    context_t *next_ctx = (context_t *)next->rsp;
    asm volatile("sti");
    switch_to_task(current_task->rsp, next_cr3);
    return (context_t *)current_task->rsp;
}

void task_exit(context_t *ctx)
{
    current_task->state = TASK_ZOMBIE;
    current_task->next->state = TASK_READY;
    schedule(ctx);
    while (1)
        asm volatile("hlt");
}
