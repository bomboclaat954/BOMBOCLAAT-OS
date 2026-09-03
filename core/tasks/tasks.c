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

#include <tasks/tasks.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <x86_64/gdt_tss.h>
#include <bomboclaat/elf64.h>
#include <bomboclaat/types.h>
#include <bomboclaat/kprintf.h>
#include <lib/string.h>
#include <fs/tmpfs.h>

extern vmm_table_t *kernel_pml4_virt;
extern uint64_t hhdm_offset;
extern int elf_alloc(void *elf_data, task_t *task, ELF64_Phdr *ph_table, uint16_t e_phnum);
extern void ret_from_fork();

task_t *tasks[MAX_TASKS] = {NULL};
task_t *current_task = NULL;
task_t *task_to_reap = NULL;
int next_pid = 1;

uintptr_t build_kernel_frame(task_t *task)
{
    uintptr_t base = (task->kstack_top - sizeof(context_t) - 144) & ~0xFULL;
    uint64_t *dummy_regs = (uint64_t *)base;

    for (int i = 0; i < 15; i++)
        dummy_regs[i] = 0;

    *(uint64_t *)(base + 136) = (uint64_t)ret_from_fork;
    *(context_t *)(base + 144) = task->cpu_ctx;

    return base;
}

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

int task_insert(task_t *t)
{
    int slot = find_free_slot();
    if (slot < 0)
        return -1;

    tasks[slot] = t;
    return 0;
}

task_t *find_just_forked()
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i] && tasks[i]->state == TASK_NEW)
            return tasks[i];
    }
    return NULL;
}

void kernel_idle_loop()
{
    log(LOG_INFO, "Entering kernel idle loop");
    while (1)
        asm volatile("hlt");
}

void task_init(void)
{
    task_t *kernel_task = (task_t *)kmalloc(sizeof(task_t));
    memset(kernel_task, 0, sizeof(task_t));

    kernel_task->pid = 0;
    strcpy("kernel", kernel_task->name);
    kernel_task->state = TASK_BLOCKED;
    kernel_task->pml4 = kernel_pml4_virt;

    for (int i = 0; i < 4; i++)
    {
        void *frame = pmm_alloc_frame();
        kernel_task->kstack_frames[i] = (uintptr_t)frame;
    }
    kernel_task->kstack_top = kernel_task->kstack_frames[3] + hhdm_offset + PAGE_SIZE;
    *(uint64_t *)(kernel_task->kstack_frames[3] + hhdm_offset + 8) = TASK_STACK_SENTINEL;
    kernel_task->kernel_rsp = build_kernel_frame(kernel_task);

    kernel_task->cpu_ctx.cs = 0x28;
    kernel_task->cpu_ctx.ss = 0x30;
    kernel_task->cpu_ctx.rflags = 0x202;
    kernel_task->cpu_ctx.rsp = kernel_task->kstack_top;
    kernel_task->cpu_ctx.rip = (uint64_t)kernel_idle_loop;

    kernel_task->next = kernel_task;
    kernel_task->parent = kernel_task;
    current_task = kernel_task;
    tasks[0] = kernel_task;

    INIT_LIST_HEAD(&kernel_task->children);
}

task_t *task_create(void *elf_data, int parent_pid, char *name, int argc, char **argv, int frames)
{
    ELF64_Ehdr *header = (ELF64_Ehdr *)elf_data;
    if (*(uint32_t *)header->e_ident != ELF_MAGIC || header->e_machine != 0x3E)
        return NULL;

    task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
    if (!new_task)
        return NULL;
    memset(new_task, 0, sizeof(task_t));

    strcpy(name, new_task->name);
    new_task->pid = next_pid++;
    new_task->parent = current_task;
    new_task->pml4 = vmm_init();

    INIT_LIST_HEAD(&new_task->children);
    list_add_tail(&new_task->sibling, &current_task->children);

    ELF64_Phdr *ph_table = (ELF64_Phdr *)((uintptr_t)elf_data + header->e_phoff);
    if (elf_alloc(elf_data, new_task, ph_table, header->e_phnum) != 0)
    {
        kfree(new_task);
        return NULL;
    }

    uintptr_t user_stack_virtual = 0xFFFFFFF000000000;

    void *top_frame_phys = NULL;
    for (int i = 0; i < frames; i++)
    {
        void *frame_phys = pmm_alloc_frame();
        if (!frame_phys)
        {
            kfree(new_task);
            return NULL;
        }

        if (vmm_map_page(new_task->pml4, user_stack_virtual + (i * PAGE_SIZE), (uintptr_t)frame_phys, VMM_PRESENT | VMM_WRITE | VMM_USER) != 0)
        {
            kfree(new_task);
            return NULL;
        }

        if (i == frames - 1)
            top_frame_phys = frame_phys;
    }

    uintptr_t fbf_virtual = 0x7FFF00000000;

    extern uintptr_t fbf_phys;
    extern uintptr_t fbf_size;
    size_t aligned_size = (fbf_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (size_t offset = 0; offset < aligned_size; offset += PAGE_SIZE)
    {
        uintptr_t phys = fbf_phys + offset;
        uintptr_t virt = fbf_virtual + offset;
        vmm_map_page(new_task->pml4, virt, phys, VMM_PRESENT | VMM_WRITE | VMM_USER);
    }

    uintptr_t top_frame_kvirt = (uintptr_t)top_frame_phys + hhdm_offset;
    uintptr_t frame_offset = PAGE_SIZE;

    uintptr_t *user_argv_addrs = (uintptr_t *)kmalloc(sizeof(uintptr_t) * (argc + 1));
    if (!user_argv_addrs)
    {
        kfree(new_task);
        return NULL;
    }

    for (int i = argc - 1; i >= 0; i--)
    {
        size_t len = argv[i] ? (strlen(argv[i]) + 1) : 0;

        if (len > frame_offset)
        {
            kfree(user_argv_addrs);
            kfree(new_task);
            return NULL;
        }

        frame_offset -= len;

        char *dest = (char *)(top_frame_kvirt + frame_offset);
        if (argv[i])
            memcpy(dest, argv[i], strlen(argv[i]));

        user_argv_addrs[i] = (user_stack_virtual + (frames * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);
    }
    user_argv_addrs[argc] = (uintptr_t)NULL;

    frame_offset &= ~0xFULL;

    size_t argv_array_size = sizeof(uintptr_t) * (argc + 1);
    frame_offset -= argv_array_size;

    uintptr_t *dest_argv_array = (uintptr_t *)(top_frame_kvirt + frame_offset);
    memcpy((uint8_t *)dest_argv_array, (uint8_t *)user_argv_addrs, argv_array_size);

    uintptr_t user_argv_ptr = (user_stack_virtual + (frames * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);

    frame_offset -= sizeof(uintptr_t);
    uintptr_t *dest_argc = (uintptr_t *)(top_frame_kvirt + frame_offset);
    *dest_argc = (uintptr_t)argc;

    uintptr_t user_rsp = (user_stack_virtual + (frames * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);

    kfree(user_argv_addrs);

    for (int i = 0; i < 4; i++)
    {
        void *kstack_phys = pmm_alloc_frame();
        if (!kstack_phys)
        {
            kfree(new_task);
            return NULL;
        }
        new_task->kstack_frames[i] = (uintptr_t)kstack_phys;
    }
    new_task->kstack_top = new_task->kstack_frames[3] + hhdm_offset + PAGE_SIZE;
    *(uint64_t *)(new_task->kstack_frames[3] + hhdm_offset + 8) = TASK_STACK_SENTINEL;
    new_task->kernel_rsp = build_kernel_frame(new_task);

    new_task->cpu_ctx.rip = header->e_entry;
    new_task->cpu_ctx.rsp = user_rsp;
    new_task->cpu_ctx.cs = 0x43;
    new_task->cpu_ctx.ss = 0x3B;
    new_task->cpu_ctx.rflags = 0x202;
    new_task->cpu_ctx.rdi = argc;
    new_task->cpu_ctx.rsi = user_argv_ptr;

    new_task->next = current_task->next;
    current_task->next = new_task;
    new_task->state = TASK_READY;

    if (task_insert(new_task) < 0)
    {
        kfree(new_task);
        return NULL;
    }

    return new_task;
}

void unlink_from_runqueue(task_t *victim)
{
    task_t *node = current_task;
    do
    {
        if (node->next == victim)
        {
            node->next = victim->next;
            break;
        }
        node = node->next;
    } while (node != current_task);
}

void reap_zombie(void)
{
    if (task_to_reap == NULL || task_to_reap == current_task)
        return;

    task_t *zombie = task_to_reap;
    task_to_reap = NULL;

    unlink_from_runqueue(zombie);
    pmm_free_frame((void *)((uintptr_t)zombie->pml4 - hhdm_offset));

    for (int i = 0; i < 4; i++)
        pmm_free_frame((void *)zombie->kstack_frames[i]);

    kfree(zombie);
}

pid_t new_pid()
{
    return next_pid++;
}
