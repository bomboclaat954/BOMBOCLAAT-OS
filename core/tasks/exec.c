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
// Por qué Dios... Por qué?...
#include <x86_64/gdt_tss.h>
#include <tasks/tasks.h>
#include <tasks/exec.h>
#include <memory/pmm.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <memory/vmm.h>
#include <lib/string.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/elf64.h>
#include <bomboclaat/globals.h>

#define USER_STACK_PAGES 4

extern uintptr_t build_kernel_frame_from_syscall(task_t *task);
extern void set_syscall_kernel_stack(uint64_t rsp);
extern tss_ptr tss;

int calculate_argc(char **argv)
{
    if (!argv)
        return 0;
    int argc = 0;
    while (argv[argc])
        argc++;

    return argc;
}

/**
    * These arguments names may look like Gibberish, so here's an explaination:
    @param elf_data: raw ELF file data
    @param shoff: offset of the sections table
    @param shentsz: size of one entry in the sections table
    @param shnum: number of total sections table entries
    @param shstrndx: index of the string table containing sections names
*/
int prepare_sections(void *elf_data, uint64_t shoff, uint16_t shentsz, uint16_t shnum, uint16_t shstrndx)
{
    // TODO: find and clear .bss
    ELF64_Shdr *shstrtab_hdr = (ELF64_Shdr *)(elf_data + shoff + (shentsz * shstrndx));
    char *strtab = (char *)(elf_data + shstrtab_hdr->sh_offset);

    for (int i = 0; i < shnum; i++)
    {
        ELF64_Shdr *shdr = (ELF64_Shdr *)(elf_data + shoff + (shentsz * i));
        char *name = strtab + shdr->sh_name;
    }

    return 0;
}

int elf_alloc(void *elf_data, task_t *task, ELF64_Phdr *ph_table, uint16_t e_phnum)
{
    task->mm.code_start = UINTPTR_MAX;
    task->mm.code_end = 0;
    task->mm.data_start = UINTPTR_MAX;
    task->mm.data_end = 0;

    for (int i = 0; i < e_phnum; i++)
    {
        ELF64_Phdr *phdr = &ph_table[i];
        if (phdr->p_type == PT_LOAD)
        {
            uintptr_t seg_start = phdr->p_vaddr;
            uintptr_t seg_end = phdr->p_vaddr + phdr->p_memsz;

            if (phdr->p_flags & PF_X)
            {
                if (seg_start < task->mm.code_start)
                    task->mm.code_start = seg_start;
                if (seg_end > task->mm.code_end)
                    task->mm.code_end = seg_end;
            }
            else
            {
                if (seg_start < task->mm.data_start)
                    task->mm.data_start = seg_start;
                if (seg_end > task->mm.data_end)
                    task->mm.data_end = seg_end;
            }

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

                if (vmm_map_page(task->pml4, current_virt, (uintptr_t)phys_frame, VMM_PRESENT | VMM_USER) != 0)
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

    if (task->mm.code_start == UINTPTR_MAX)
        task->mm.code_start = 0;
    if (task->mm.data_start == UINTPTR_MAX)
        task->mm.data_start = 0;

    uintptr_t highest = task->mm.code_end > task->mm.data_end ? task->mm.code_end : task->mm.data_end;
    task->mm.brk_start = (highest + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    task->mm.brk = task->mm.brk_start;

    return 0;
}

int prepare_stack(task_t *task, int argc, char **argv, uint64_t e_entry)
{
    uintptr_t user_stack_virtual = 0xFFFFFFF000000000;
    task->mm.stack_start = user_stack_virtual + (USER_STACK_PAGES * PAGE_SIZE);

    void *top_frame_phys = NULL;
    for (int i = 0; i < USER_STACK_PAGES; i++)
    {
        void *frame_phys = pmm_alloc_frame();
        if (!frame_phys)
            return 1;

        if (vmm_map_page(task->pml4, user_stack_virtual + (i * PAGE_SIZE), (uintptr_t)frame_phys, VMM_PRESENT | VMM_WRITE | VMM_USER) != 0)
            return 2;

        if (i == USER_STACK_PAGES - 1)
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
        return 3;
    }
    uintptr_t top_frame_kvirt = (uintptr_t)top_frame_phys + hhdm_offset;
    uintptr_t frame_offset = PAGE_SIZE;

    for (int i = argc - 1; i >= 0; i--)
    {
        size_t len = argv[i] ? (strlen(argv[i]) + 1) : 0;

        if (len > frame_offset)
        {
            kfree(user_argv_addrs);
            return 4; /* OH SHIT ARGV TOO BIG */
        }

        frame_offset -= len;

        char *dest = (char *)(top_frame_kvirt + frame_offset);
        if (argv[i])
            memcpy(dest, argv[i], strlen(argv[i]));

        user_argv_addrs[i] = (user_stack_virtual + (USER_STACK_PAGES * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);
    }
    user_argv_addrs[argc] = (uintptr_t)NULL;

    frame_offset &= ~0xFULL;

    size_t argv_array_size = sizeof(uintptr_t) * (argc + 1);
    frame_offset -= argv_array_size;

    // uintptr_t *dest_argv_array = (uintptr_t *)(k_stack_high - (PAGE_SIZE - frame_offset));
    uintptr_t *dest_argv_array = (uintptr_t *)(top_frame_kvirt + frame_offset);
    memcpy((uint8_t *)dest_argv_array, (uint8_t *)user_argv_addrs, argv_array_size);

    uintptr_t user_argv_ptr = (user_stack_virtual + (USER_STACK_PAGES * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);

    frame_offset -= sizeof(uintptr_t);
    uintptr_t *dest_argc = (uintptr_t *)(top_frame_kvirt + frame_offset); //(uintptr_t *)(k_stack_high - (PAGE_SIZE - frame_offset));
    *dest_argc = (uintptr_t)argc;

    uintptr_t user_rsp = (user_stack_virtual + (USER_STACK_PAGES * PAGE_SIZE)) - (PAGE_SIZE - frame_offset);

    /*for (int i = 0; i < 4; i++)
        pmm_free_frame((void *)task->kstack_frames[i]);

    for (int i = 0; i < 4; i++)
    {
        void *kstack_phys = pmm_alloc_frame();
        if (!kstack_phys)
        {
            kfree(user_argv_addrs);
            return 5;
        }
        task->kstack_frames[i] = (uintptr_t)kstack_phys;
    }
    task->kstack_top = task->kstack_frames[3] + hhdm_offset + PAGE_SIZE;
    *(uint64_t *)(task->kstack_frames[3] + hhdm_offset + 8) = TASK_STACK_SENTINEL;*/

    context_t ctx = task->parent->cpu_ctx;
    ctx.rip = e_entry;
    ctx.rsp = user_rsp;
    ctx.cs = 0x43;
    ctx.ss = 0x3B;
    ctx.rflags = 0x202;
    ctx.rdi = argc;
    ctx.rsi = user_argv_ptr;
    task->cpu_ctx = ctx;

    kfree(user_argv_addrs);
    task->kernel_rsp = build_kernel_frame_from_syscall(task);

    return 0;
}

int execve(struct execve_args args)
{
    uint64_t size = 0;
    int fd = vfs_open(args.path, 0, &size, current_task->fd_table);
    if (fd < 0)
        return 1;

    void *file = kmalloc(size + 1);
    if (file == NULL)
    {
        vfs_close(fd, current_task->fd_table);
        return 2;
    }

    int64_t read_bytes = vfs_read(fd, current_task->fd_table, file, size);
    vfs_close(fd, current_task->fd_table);
    if (read_bytes <= 0)
        return 3;

    ELF64_Ehdr *header = (ELF64_Ehdr *)file;
    if (*(uint32_t *)header->e_ident != ELF_MAGIC || header->e_machine != 0x3E)
    {
        kfree(file);
        return 4;
    }

    if (header->e_machine != 0x3E)
    {
        log(LOG_ERR, "This ELF file is not runable on x86_64");
        return 5;
    }

    // prepare_sections(file, header->e_shoff, header->e_shentsize, header->e_shnum, header->e_shstrndx);

    vmm_table_t *old_pml4 = current_task->pml4;
    current_task->pml4 = vmm_init();

    strcpy(args.path, current_task->name);

    ELF64_Phdr *ph_table = (ELF64_Phdr *)((uintptr_t)file + header->e_phoff);
    if (elf_alloc(file, current_task, ph_table, header->e_phnum) != 0)
    {
        kfree(file);
        vmm_free(current_task->pml4);
        current_task->pml4 = old_pml4;
        return 6;
    }
    kfree(file);
    file = NULL;

    int argc = calculate_argc(args.argv);
    if (prepare_stack(current_task, argc, args.argv, header->e_entry) != 0)
    {
        vmm_free(current_task->pml4);
        current_task->pml4 = old_pml4;
        return 7;
    }
    vmm_free(old_pml4);

    asm volatile("cli");
    tss.rsp0 = current_task->kstack_top;
    set_syscall_kernel_stack(current_task->kstack_top);
    vmm_switch_pml4(current_task->pml4);
    enter_new_context(current_task->kernel_rsp);

    return 0;
}
