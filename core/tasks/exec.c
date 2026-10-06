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
#include <memory/userspace.h>
#include <memory/vmm.h>
#include <lib/string.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/elf64.h>
#include <bomboclaat/globals.h>
#include <errno.h>

#define EXEC_MAX_ARGS 32
#define EXEC_MAX_PATH 256
#define EXEC_ARG_BYTES 3584
#define EXEC_MAX_FILE (16 * 1024 * 1024)
#define EXEC_MAX_SEGMENT (64ULL * 1024 * 1024)
#define EXEC_MAX_PHNUM 64
#define EXEC_MAX_SHNUM 256

extern void set_syscall_kernel_stack(uint64_t rsp);
extern uint64_t hhdm_offset;

struct exec_args_k
{
    char path[EXEC_MAX_PATH];
    char *argv[EXEC_MAX_ARGS + 1];
    char buf[EXEC_ARG_BYTES];
    int argc;
};

static uint64_t nx_flag(void)
{
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000080));
    return (lo & (1U << 11)) ? VMM_NX : 0;
}

static int elf_validate(const uint8_t *data, size_t size)
{
    if (!data || size < sizeof(ELF64_Ehdr))
        return -ENOEXEC;

    const ELF64_Ehdr *eh = (const ELF64_Ehdr *)data;

    if (*(const uint32_t *)eh->e_ident != ELF_MAGIC || eh->e_ident[4] != 2 || eh->e_ident[5] != 1)
        return -ENOEXEC;
    if (eh->e_machine != 0x3E || eh->e_type != 2)
        return -ENOEXEC;
    if (eh->e_phentsize != sizeof(ELF64_Phdr) || eh->e_phnum == 0 || eh->e_phnum > EXEC_MAX_PHNUM)
        return -ENOEXEC;
    if (eh->e_phoff > size || (uint64_t)eh->e_phnum * sizeof(ELF64_Phdr) > size - eh->e_phoff)
        return -ENOEXEC;
    if (eh->e_entry < USER_MIN_ADDR || eh->e_entry >= USER_IMAGE_LIMIT)
        return -ENOEXEC;

    const ELF64_Phdr *ph = (const ELF64_Phdr *)(data + eh->e_phoff);
    int loads = 0;
    int entry_ok = 0;

    for (int i = 0; i < eh->e_phnum; i++)
    {
        if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
            continue;

        if (ph[i].p_filesz > ph[i].p_memsz || ph[i].p_memsz > EXEC_MAX_SEGMENT)
            return -ENOEXEC;
        if (ph[i].p_offset > size || ph[i].p_filesz > size - ph[i].p_offset)
            return -ENOEXEC;
        if (ph[i].p_vaddr < USER_MIN_ADDR || ph[i].p_vaddr >= USER_IMAGE_LIMIT)
            return -ENOEXEC;
        if (ph[i].p_memsz > USER_IMAGE_LIMIT - ph[i].p_vaddr)
            return -ENOEXEC;

        if ((ph[i].p_flags & PF_X) && eh->e_entry >= ph[i].p_vaddr && eh->e_entry < ph[i].p_vaddr + ph[i].p_memsz)
            entry_ok = 1;
        loads++;
    }

    if (!loads || !entry_ok)
        return -ENOEXEC;

    return 0;
}

static uint64_t elf_find_sigterm(const uint8_t *data, size_t size)
{
    const ELF64_Ehdr *eh = (const ELF64_Ehdr *)data;

    if (eh->e_shoff == 0 || eh->e_shnum == 0 || eh->e_shnum > EXEC_MAX_SHNUM)
        return 0;
    if (eh->e_shentsize != sizeof(ELF64_Shdr) || eh->e_shstrndx >= eh->e_shnum)
        return 0;
    if (eh->e_shoff > size || (uint64_t)eh->e_shnum * sizeof(ELF64_Shdr) > size - eh->e_shoff)
        return 0;

    const ELF64_Shdr *sh = (const ELF64_Shdr *)(data + eh->e_shoff);
    const ELF64_Shdr *strsec = &sh[eh->e_shstrndx];

    if (strsec->sh_offset > size || strsec->sh_size > size - strsec->sh_offset)
        return 0;

    const char *strtab = (const char *)(data + strsec->sh_offset);
    static const char wanted[] = ".sigterm";

    for (int i = 0; i < eh->e_shnum; i++)
    {
        uint64_t off = sh[i].sh_name;
        if (off >= strsec->sh_size || strsec->sh_size - off < sizeof(wanted))
            continue;

        int same = 1;
        for (size_t k = 0; k < sizeof(wanted); k++)
        {
            if (strtab[off + k] != wanted[k])
            {
                same = 0;
                break;
            }
        }

        if (same && sh[i].sh_addr >= USER_MIN_ADDR && sh[i].sh_addr < USER_IMAGE_LIMIT)
            return sh[i].sh_addr;
    }

    return 0;
}

static int elf_load(const uint8_t *data, vmm_table_t *pml4, mm_t *mm)
{
    const ELF64_Ehdr *eh = (const ELF64_Ehdr *)data;
    const ELF64_Phdr *ph = (const ELF64_Phdr *)(data + eh->e_phoff);
    uint64_t nx = nx_flag();

    mm->code_start = UINTPTR_MAX;
    mm->code_end = 0;
    mm->data_start = UINTPTR_MAX;
    mm->data_end = 0;

    for (int i = 0; i < eh->e_phnum; i++)
    {
        const ELF64_Phdr *seg = &ph[i];
        if (seg->p_type != PT_LOAD || seg->p_memsz == 0)
            continue;

        uintptr_t seg_start = seg->p_vaddr;
        uintptr_t seg_end = seg->p_vaddr + seg->p_memsz;

        if (seg->p_flags & PF_X)
        {
            if (seg_start < mm->code_start)
                mm->code_start = seg_start;
            if (seg_end > mm->code_end)
                mm->code_end = seg_end;
        }
        else
        {
            if (seg_start < mm->data_start)
                mm->data_start = seg_start;
            if (seg_end > mm->data_end)
                mm->data_end = seg_end;
        }

        uint64_t flags = VMM_PRESENT | VMM_USER;
        if (seg->p_flags & PF_W)
            flags |= VMM_WRITE;
        if (!(seg->p_flags & PF_X))
            flags |= nx;

        uintptr_t first = seg_start & ~(uintptr_t)(PAGE_SIZE - 1);
        uintptr_t last = (seg_end + PAGE_SIZE - 1) & ~(uintptr_t)(PAGE_SIZE - 1);

        for (uintptr_t va = first; va < last; va += PAGE_SIZE)
        {
            uintptr_t phys = 0;
            uint64_t old_flags = 0;
            uint8_t *kvirt;

            if (vmm_resolve(pml4, va, &phys, &old_flags) == 0)
            {
                phys &= ~(uintptr_t)(PAGE_SIZE - 1);
                uint64_t merged = VMM_PRESENT | VMM_USER | (old_flags & VMM_WRITE) | (flags & VMM_WRITE);
                if (vmm_map_page(pml4, va, phys, merged) != 0)
                    return -ENOMEM;
            }
            else
            {
                void *frame = pmm_alloc_frame_zeroed();
                if (!frame)
                    return -ENOMEM;
                phys = (uintptr_t)frame;
                if (vmm_map_page(pml4, va, phys, flags) != 0)
                {
                    pmm_free_frame(frame);
                    return -ENOMEM;
                }
            }

            kvirt = (uint8_t *)(phys + hhdm_offset);

            uintptr_t file_end = seg_start + seg->p_filesz;
            uintptr_t lo = va > seg_start ? va : seg_start;
            uintptr_t hi = (va + PAGE_SIZE) < file_end ? (va + PAGE_SIZE) : file_end;

            if (lo < hi)
                memcpy(kvirt + (lo - va), (uint8_t *)data + seg->p_offset + (lo - seg_start), (uint32_t)(hi - lo));
        }
    }

    if (mm->code_start == UINTPTR_MAX)
        mm->code_start = 0;
    if (mm->data_start == UINTPTR_MAX)
        mm->data_start = 0;

    uintptr_t highest = mm->code_end > mm->data_end ? mm->code_end : mm->data_end;
    mm->brk_start = (highest + PAGE_SIZE - 1) & ~(uintptr_t)(PAGE_SIZE - 1);
    mm->brk = mm->brk_start;

    return 0;
}

int image_build(const void *elf, size_t size, int argc, char **argv, int stack_pages, struct image *out)
{
    memset(out, 0, sizeof(struct image));

    int rc = elf_validate((const uint8_t *)elf, size);
    if (rc != 0)
        return rc;

    out->pml4 = vmm_init();
    if (!out->pml4)
        return -ENOMEM;

    rc = elf_load((const uint8_t *)elf, out->pml4, &out->mm);
    if (rc == 0)
        rc = task_setup_user_stack(out->pml4, &out->mm, argc, argv, stack_pages, &out->rsp, &out->argv);
    if (rc == 0)
        rc = task_map_shared_regions(out->pml4);

    if (rc != 0)
    {
        vmm_free(out->pml4);
        out->pml4 = NULL;
        return rc;
    }

    out->entry = ((const ELF64_Ehdr *)elf)->e_entry;
    out->argc = (uint64_t)argc;
    out->sigterm = elf_find_sigterm((const uint8_t *)elf, size);
    return 0;
}

void image_free(struct image *img)
{
    if (img && img->pml4)
    {
        vmm_free(img->pml4);
        img->pml4 = NULL;
    }
}

static int load_file(const char *path, void **data, size_t *size)
{
    uint64_t file_size = 0;

    int fd = vfs_open((char *)path, 0, &file_size, current_task->fd_table);
    if (fd < 0)
        return -ENOENT;

    if (file_size == 0 || file_size > EXEC_MAX_FILE)
    {
        vfs_close(fd, current_task->fd_table);
        return -ENOEXEC;
    }

    uint8_t *buf = (uint8_t *)kmalloc(file_size + 1);
    if (!buf)
    {
        vfs_close(fd, current_task->fd_table);
        return -ENOMEM;
    }

    uint64_t total = 0;
    while (total < file_size)
    {
        int n = vfs_read(fd, current_task->fd_table, buf + total, file_size - total);
        if (n <= 0)
            break;
        total += (uint64_t)n;
    }
    vfs_close(fd, current_task->fd_table);

    if (total != file_size)
    {
        kfree(buf);
        return -EIO;
    }

    *data = buf;
    *size = (size_t)file_size;
    return 0;
}

static int exec_copy_args(struct exec_args_k *ka, struct execve_args args)
{
    memset(ka, 0, sizeof(*ka));

    if (copy_string_from_user(ka->path, args.path, EXEC_MAX_PATH) < 0)
        return -EFAULT;
    if (ka->path[0] != '/')
        return -ENOENT;

    size_t used = 0;
    ka->argc = 0;

    if (!args.argv)
        return 0;

    for (int i = 0; i < EXEC_MAX_ARGS; i++)
    {
        uint64_t user_ptr = 0;
        if (copy_from_user(&user_ptr, (void *)((uintptr_t)args.argv + (uintptr_t)i * sizeof(uint64_t)), sizeof(user_ptr)) != 0)
            return -EFAULT;
        if (user_ptr == 0)
            return 0;

        int len = copy_string_from_user(ka->buf + used, (const char *)user_ptr, (uint32_t)(EXEC_ARG_BYTES - used));
        if (len < 0)
            return -E2BIG;

        ka->argv[i] = ka->buf + used;
        used += (size_t)len + 1;
        ka->argc = i + 1;
    }

    return -E2BIG;
}

int execve(struct execve_args args)
{
    struct exec_args_k *ka = (struct exec_args_k *)kmalloc(sizeof(struct exec_args_k));
    if (!ka)
        return -ENOMEM;

    int rc = exec_copy_args(ka, args);
    if (rc != 0)
    {
        kfree(ka);
        return rc;
    }

    void *data = NULL;
    size_t size = 0;
    rc = load_file(ka->path, &data, &size);
    if (rc != 0)
    {
        kfree(ka);
        return rc;
    }

    struct image img;
    rc = image_build(data, size, ka->argc, ka->argv, USER_STACK_PAGES, &img);
    kfree(data);
    if (rc != 0)
    {
        kfree(ka);
        return rc;
    }

    task_t *t = current_task;
    asm volatile("cli" ::: "memory");

    vmm_table_t *old_pml4 = t->pml4;
    t->pml4 = img.pml4;
    t->mm = img.mm;
    t->sigterm_handler_rip = img.sigterm;
    t->pending_signal = 0;
    task_set_name(t, ka->path);
    task_setup_user_context(t, &img);
    kfree(ka);

    task_fpu_init(t);
    task_fpu_load(t);

    tss.rsp0 = t->kstack_top;
    set_syscall_kernel_stack(t->kstack_top);
    vmm_switch_pml4(t->pml4);
    vmm_free(old_pml4);

    enter_user_context(&t->cpu_ctx, t->kstack_top);
}

int spawn(struct execve_args args)
{
    struct exec_args_k *ka = (struct exec_args_k *)kmalloc(sizeof(struct exec_args_k));
    if (!ka)
        return -ENOMEM;

    int rc = exec_copy_args(ka, args);
    if (rc != 0)
    {
        kfree(ka);
        return rc;
    }

    void *data = NULL;
    size_t size = 0;
    rc = load_file(ka->path, &data, &size);
    if (rc != 0)
    {
        kfree(ka);
        return rc;
    }

    uint64_t flags = irq_save();
    int result;
    struct image img;

    rc = image_build(data, size, ka->argc, ka->argv, USER_STACK_PAGES, &img);
    if (rc != 0)
        result = rc;
    else
    {
        task_t *child = task_from_image(current_task, &img, ka->path);
        if (!child)
        {
            image_free(&img);
            result = -EAGAIN;
        }
        else
            result = child->pid;
    }

    irq_restore(flags);
    kfree(data);
    kfree(ka);
    return result;
}
