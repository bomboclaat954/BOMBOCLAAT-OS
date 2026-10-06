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

#include <stdint.h>
#include <stddef.h>
#include <boot/limine.h>
#include <lib/string.h>
#include <memory/memtools.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/kmalloc.h>
#include <memory/stack.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/initramfs.h>
#include <bomboclaat/types.h>
#include <tasks/tasks.h>
#include <tasks/fork.h>
#include <tasks/exec.h>
#include <fs/tmpfs.h>

tmpfs_file_t **initramfs_files;
extern tmpfs_dir_t *tmpfs_root;

uint32_t parse_hex(const char *str)
{
    uint32_t result = 0;
    for (int i = 0; i < 8; i++)
    {
        result <<= 4;
        if (str[i] >= '0' && str[i] <= '9')
            result |= (str[i] - '0');
        else if (str[i] >= 'a' && str[i] <= 'f')
            result |= (str[i] - 'a' + 10);
        else if (str[i] >= 'A' && str[i] <= 'F')
            result |= (str[i] - 'A' + 10);
    }
    return result;
}

uint64_t initramfs_get_files(void *start, tmpfs_file_t **out_buf, uint64_t max_files)
{
    uint8_t *ptr = (uint8_t *)start;
    uint64_t idx = 0;

    while (idx < max_files)
    {
        struct cpio_header *header = (struct cpio_header *)ptr;

        if (strncmp(header->c_magic, "070701", 6) != 0)
            break;

        uint32_t file_size = parse_hex(header->c_filesize);
        uint32_t name_size = parse_hex(header->c_namesize);
        char *file_name = (char *)(ptr + 110);
        uint32_t data_offset = (110 + name_size + 3) & ~3;

        if (strcmp(file_name, "TRAILER!!!") == 0)
            break;

        uint8_t *fileData = ptr + data_offset;
        out_buf[idx] = (tmpfs_file_t *)kmalloc(sizeof(tmpfs_file_t));
        out_buf[idx]->content = fileData;
        out_buf[idx]->dir = tmpfs_root;
        out_buf[idx]->name = join("/", file_name, 0);
        out_buf[idx]->size = file_size;

        uint32_t next_file_offset = data_offset + ((file_size + 3) & ~3);
        ptr += next_file_offset;
        idx++;
    }

    if (idx == max_files)
        panic("initramfs: too many entries for buffer", 0, 0);

    return idx;
}

void initramfs()
{
    if (module_request.response == NULL || module_request.response->module_count == 0)
        panic("didn't get any modules from Limine", 0, 0);

    void *initramfs_base = NULL;

    for (uint64_t i = 0; i < module_request.response->module_count; i++)
    {
        struct limine_file *file = module_request.response->modules[i];

        if (file->string != NULL && strcmp(file->string, "initramfs") == 0)
            initramfs_base = file->address;
    }

    if (initramfs_base == NULL)
        panic("didn't find initramfs module", 0, 0);

    initramfs_files = kmalloc(512 * sizeof(tmpfs_file_t *));
    memset(initramfs_files, 0, 512 * sizeof(tmpfs_file_t *));

    uint64_t file_count = initramfs_get_files(initramfs_base, initramfs_files, 512);

    for (uint64_t x = 0; x < file_count; x++)
    {
        tmpfs_file_t *file = initramfs_files[x];
        INIT_LIST_HEAD(&file->node);
        list_add_tail(&file->node, &tmpfs_root->files);
    }
    tmpfs_root->nfiles = file_count;

    int init_pos = -1;
    uint64_t init_size = 0;

    for (int i = 0; i < file_count; i++)
    {
        if (strcmp("/bin/init", initramfs_files[i]->name) == 0)
        {
            init_pos = i;
            init_size = initramfs_files[i]->size;
            break;
        }
    }

    if (init_pos == -1)
        panic("didn't find /bin/init in initramfs", 0, 0);

    void *init_data = initramfs_files[init_pos]->content;

    /*
        THEORETICALLY I could use fork() and execve() here (because the kernel task is already running),
        but I'm a bit afraid it won't work so I'll keep it like that.
    */
    task_t *init_task = task_create(init_data, init_size, 0, "/bin/init", 0, 0, USER_STACK_PAGES);

    if (init_task == NULL)
        panic("Failed to create init process", 0, 0);
}
