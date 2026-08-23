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
#include <tasks/exec.h>
#include <memory/pmm.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <memory/vmm.h>
#include <lib/string.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/elf64.h>
#include <errno.h>

extern task_t *current_task;
extern uint64_t hhdm_offset;

#define USER_STACK_PAGES 4

int elf_alloc(void *elf_data, task_t *task, ELF64_Phdr *ph_table, uint16_t e_phnum)
{
    for (int i = 0; i < e_phnum; i++)
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
                if (!phys_frame)
                    return 1;

                if (vmm_map_page(task->pml4, current_virt, (uintptr_t)phys_frame, VMM_PRESENT | VMM_WRITE | VMM_USER) != 0)
                    return 2;

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
    return 0;
}

context_t *prepare_stack(task_t *task, int frames, int argc, char **argv, uint64_t e_entry)
{
    uintptr_t user_stack_virtual = 0xFFFFFFF000000000;

    void *top_frame_phys = NULL;
    for (int i = 0; i < frames; i++)
    {
        void *frame_phys = pmm_alloc_frame();
        if (!frame_phys)
            return NULL;

        if (vmm_map_page(task->pml4, user_stack_virtual + (i * PAGE_SIZE), (uintptr_t)frame_phys, VMM_PRESENT | VMM_WRITE | VMM_USER) != 0)
            return NULL;

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
        vmm_map_page(task->pml4, virt, phys, VMM_PRESENT | VMM_WRITE | VMM_USER);
    }

    uintptr_t k_stack_high = (uintptr_t)top_frame_phys + hhdm_offset + PAGE_SIZE;
    uintptr_t *user_argv_addrs = (uintptr_t *)kmalloc(sizeof(uintptr_t) * (argc + 1));
    if (!user_argv_addrs)
    {
        log(LOG_ERR, "prepare_stack(): kmalloc error");
        return NULL;
    }
    uintptr_t top_frame_kvirt = (uintptr_t)top_frame_phys + hhdm_offset;
    uintptr_t frame_offset = PAGE_SIZE;

    for (int i = argc - 1; i >= 0; i--)
    {
        size_t len = argv[i] ? (strlen(argv[i]) + 1) : 0;

        if (len > frame_offset)
        {
            kfree(user_argv_addrs);
            return NULL; /* OH SHIT ARGV TOO BIG */
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

    // uintptr_t *dest_argv_array = (uintptr_t *)(k_stack_high - (PAGE_SIZE - frame_offset));
    uintptr_t *dest_argv_array = (uintptr_t *)(top_frame_kvirt + frame_offset);
    memcpy((uint8_t *)dest_argv_array, (uint8_t *)user_argv_addrs, argv_array_size);

    uintptr_t user_argv_ptr = (user_stack_virtual + (frames * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);

    frame_offset -= sizeof(uintptr_t);
    uintptr_t *dest_argc = (uintptr_t *)(top_frame_kvirt + frame_offset); //(uintptr_t *)(k_stack_high - (PAGE_SIZE - frame_offset));
    *dest_argc = (uintptr_t)argc;

    uintptr_t user_rsp = (user_stack_virtual + (frames * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);

    for (int i = 0; i < 4; i++)
    {
        void *kstack_phys = pmm_alloc_frame();
        log(LOG_DEBUG, "PREPARE_STACK: allocated frame address: %x", kstack_phys);
        if (!kstack_phys)
        {
            kfree(user_argv_addrs);
            return NULL;
        }
        task->kstack_frames[i] = (uintptr_t)kstack_phys;
    }
    task->kstack_top = task->kstack_frames[3] + hhdm_offset + PAGE_SIZE;

    kfree(user_argv_addrs);
    *(uint64_t *)(task->kstack_frames[3] + hhdm_offset + 8) = TASK_STACK_SENTINEL; // will be checked later, don't worry

    task->rsp = task->kstack_top - sizeof(context_t);
    context_t *ctx = (context_t *)task->rsp;
    memset(ctx, 0, sizeof(context_t));

    ctx->rip = e_entry;
    ctx->rsp = user_rsp;
    ctx->cs = 0x43;
    ctx->ss = 0x3B;
    ctx->rflags = 0x202;
    ctx->int_no = 0;
    ctx->err_code = 0;
    ctx->rdi = argc;
    ctx->rsi = user_argv_ptr;

    return ctx;
}

// For now envp isn't used but it will be
int execve(char *path, char *argv[], int argc, char *envp[])
{
    uint64_t size = 0;
    int fd = vfs_open(path, 0, &size, current_task->fd_table);
    if (fd < 0)
        return fd * -1;

    void *file = kmalloc(size + 1);
    if (file == NULL)
    {
        vfs_close(fd, current_task->fd_table);
        return 1;
    }

    int64_t read_bytes = vfs_read(fd, current_task->fd_table, file, size);
    vfs_close(fd, current_task->fd_table);
    if (read_bytes <= 0)
        return ENOENT;

    ELF64_Ehdr *header = (ELF64_Ehdr *)file;
    if (*(uint32_t *)header->e_ident != ELF_MAGIC || header->e_machine != 0x3E)
        return 2;

    int result = 0;
    task_t *new_task = find_just_forked();
    if (new_task == NULL)
    {
        result = 3;
        goto clean;
    }

    memset(new_task->name, 0, sizeof(char) * 64);
    memset(new_task->fd_table, 0, sizeof(vfs_file_t) * 32);
    memset(new_task->pml4, 0, sizeof(vmm_table_t));
    strcpy(path, new_task->name);
    new_task->pml4 = vmm_init();

    ELF64_Phdr *ph_table = (ELF64_Phdr *)((uintptr_t)file + header->e_phoff);

    if (elf_alloc(file, new_task, ph_table, header->e_phnum) != 0)
    {
        kfree(file);
        result = 4;
        goto clean;
    }
    kfree(file);
    file = NULL;

    context_t *ctx = prepare_stack(new_task, USER_STACK_PAGES, argc, argv, header->e_entry);
    if (ctx == NULL)
    {
        result = 5;
        goto clean;
    }

    new_task->next = current_task->next;
    current_task->next = new_task;
    current_task->state = TASK_BLOCKED;
    new_task->state = TASK_RUNNING;

    task_insert(new_task);
    schedule(ctx);
    return 0;

clean:
    if (file)
        kfree(file);
    for (int i = 0; i < argc; i++)
        kfree(argv[i]);
    return result;
}
