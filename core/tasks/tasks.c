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
// TODO: improve memory cleaning after the task dies
#include <tasks/tasks.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <int/int.h>
#include <bomboclaat/elf64.h>
#include <lib/string.h>

#define MAX_TASKS 32

extern vmm_table_t *kernel_pml4_virt;
extern uint64_t hhdm_offset;
extern tss_ptr tss;

task_t *tasks[MAX_TASKS] = {NULL};
task_t *current_task = NULL;
int next_pid = 1;

int find_free_slot()
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (!tasks[i])
            return i;
    }
    return -1;
}

int find_in_array(task_t *t)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i] == t)
            return i;
    }
    return -1;
}

task_t *find_by_pid(int pid)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i] != NULL && tasks[i]->pid == pid)
            return tasks[i];
    }
    return NULL;
}

void kernel_idle_loop()
{
    while (1)
        asm volatile("hlt");
}

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
    ctx->rip = (uint64_t)kernel_idle_loop;
    ctx->cs = 0x28;
    ctx->ss = 0x30;
    ctx->rflags = 0x202;
    ctx->rsp = kernel_task->kstack_top;

    kernel_task->next = kernel_task;
    kernel_task->parent_pid = 0;
    current_task = kernel_task;
    tasks[0] = kernel_task;
}

task_t *task_create(void *elf_data, int parent_pid, char *name, int argc, char **argv, int frames)
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

                    memcpy(kvirt + dest_off, (uint8_t *)elf_data + phdr->p_offset + file_bytes_written, to_copy);

                    file_bytes_written += to_copy;
                }

                bytes_written += space;
                current_virt += PAGE_SIZE;
            }
        }
    }

    uintptr_t user_stack_virtual = 0xFFFFFFFFD0000000;

    void *top_frame_phys = NULL;
    for (int i = 0; i < frames; i++)
    {
        void *frame_phys = pmm_alloc_frame();
        vmm_map_page(new_task->pml4, user_stack_virtual + (i * PAGE_SIZE), (uintptr_t)frame_phys, VMM_PRESENT | VMM_WRITE | VMM_USER);
        if (i == frames - 1)
            top_frame_phys = frame_phys;
    }

    uintptr_t k_stack_high = (uintptr_t)top_frame_phys + hhdm_offset + PAGE_SIZE;
    size_t frame_offset = PAGE_SIZE;

    uintptr_t *user_argv_addrs = (uintptr_t *)kmalloc(sizeof(uintptr_t) * (argc + 1));

    for (int i = argc - 1; i >= 0; i--)
    {
        size_t len = strlen(argv[i]) + 1;
        frame_offset -= len;

        char *dest = (char *)(k_stack_high - (PAGE_SIZE - frame_offset));
        strcpy(argv[i], dest);

        user_argv_addrs[i] = (user_stack_virtual + (frames * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);
    }
    user_argv_addrs[argc] = (uintptr_t)NULL;

    frame_offset &= ~0xFULL;

    size_t argv_array_size = sizeof(uintptr_t) * (argc + 1);
    frame_offset -= argv_array_size;

    uintptr_t *dest_argv_array = (uintptr_t *)(k_stack_high - (PAGE_SIZE - frame_offset));
    memcpy((uint8_t *)dest_argv_array, (uint8_t *)user_argv_addrs, argv_array_size);

    uintptr_t user_argv_ptr = (user_stack_virtual + (frames * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);

    frame_offset -= sizeof(uintptr_t);
    uintptr_t *dest_argc = (uintptr_t *)(k_stack_high - (PAGE_SIZE - frame_offset));
    *dest_argc = (uintptr_t)argc;

    uintptr_t user_rsp = (user_stack_virtual + (frames * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);

    kfree(user_argv_addrs);

    void *kstack_phys = NULL;
    for (int i = 0; i < 4; i++)
        kstack_phys = pmm_alloc_frame();
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
    ctx->rdi = argc;
    ctx->rsi = user_argv_ptr;

    new_task->next = current_task->next;
    current_task->next = new_task;
    new_task->state = TASK_READY;

    int slot = find_free_slot();
    if (slot < 0)
        return NULL;
    tasks[slot] = new_task;
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

    asm volatile("sti");
    switch_to_task(current_task->rsp, next_cr3);

    while (1)
        asm volatile("hlt");
}

void task_exit(context_t *ctx)
{
    current_task->state = TASK_ZOMBIE;
    task_t *prev = current_task;

    int slot = find_in_array(prev);
    if (slot >= 0)
        tasks[slot] = NULL;

    vmm_unmap_page(prev->pml4, (uintptr_t)prev->pml4 + hhdm_offset);
    pmm_free_frame((void *)((uintptr_t)prev->pml4 - hhdm_offset));
    kfree(&prev->kstack_top);
    kfree(prev);

    schedule(ctx);
    while (1)
        asm volatile("hlt");
}
