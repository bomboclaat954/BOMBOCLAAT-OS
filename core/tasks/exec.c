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

int calculate_argc(char *argv[])
{
}

// For now envp isn't used but it will be
int execve(char *path, char *argv[], char *envp[])
{
    uint64_t size = 0;
    int fd = vfs_open(path, 0, &size);
    if (fd < 0)
        return fd * -1;

    void *file = kmalloc(size + 1);
    if (file == NULL)
    {
        vfs_close(fd);
        return 1;
    }

    int64_t read_bytes = vfs_read(fd, file, size);
    vfs_close(fd);
    if (read_bytes <= 0)
        return ENOENT;

    ELF64_Ehdr *header = (ELF64_Ehdr *)file;
    if (*(uint32_t *)header->e_ident != ELF_MAGIC || header->e_machine != 0x3E) // not a valid ELF64
        return 2;

    int frames = (size + PAGE_SIZE - 1) >> 12;
    int argc = calculate_argc(argv);

    task_t *new_task = find_just_forked();
    if (new_task == NULL)
    {
        goto clean;
        return 1;
    }

    memset(new_task->name, 0, sizeof(char) * 64);
    memset(new_task->fd_table, 0, sizeof(vfs_file_t) * 32);
    strcpy(path, new_task->name); // Yes, my strcpy is reversed
    new_task->pml4 = vmm_init();

    current_task->state = TASK_BLOCKED;
    new_task->state = TASK_READY;
    goto clean;

    return 0;

clean:
    kfree(file);
    for (int i = 0; i < argc; i++)
        kfree(argv[i]);
}
