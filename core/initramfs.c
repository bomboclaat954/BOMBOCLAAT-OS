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
#include <tasks/loader.h>
#include <tasks/tasks.h>
#include <fs/tmpfs.h>

uint8_t *init_heap_current;
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

uint64_t initramfs_get_files(void *start, tmpfs_file_t **out_buf)
{
    uint8_t *ptr = (uint8_t *)start;
    uint64_t idx = 0;

    while (1)
    {
        struct cpio_header *header = (struct cpio_header *)ptr;

        if (strncmp(header->c_magic, "070701", 6) != 0)
            break;

        uint32_t fileSize = parse_hex(header->c_filesize);
        uint32_t nameSize = parse_hex(header->c_namesize);

        char *fileName = (char *)(ptr + 110);

        uint32_t dataOffset = (110 + nameSize + 3) & ~3;

        if (strcmp(fileName, "TRAILER!!!") == 0)
            break;

        uint8_t *fileData = ptr + dataOffset;
        out_buf[idx] = (tmpfs_file_t *)kmalloc(sizeof(tmpfs_file_t));
        out_buf[idx]->content = fileData;
        out_buf[idx]->dir = tmpfs_root;
        out_buf[idx]->name = fileName;
        out_buf[idx]->size = fileSize;

        uint32_t nextFileOffset = dataOffset + ((fileSize + 3) & ~3);
        ptr += nextFileOffset;
        idx++;
    }
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

    uint64_t file_count = initramfs_get_files(initramfs_base, initramfs_files);

    for (uint64_t x = 0; x < file_count; x++)
    {
        if (x >= 1024)
            panic("too many files for tmpfs root directory!", 0, 0);
        tmpfs_root->files[x] = initramfs_files[x];
    }
    tmpfs_root->files_count = file_count;

    uint64_t init_size = 0;
    uint64_t init_pos = 0;

    for (int i = 0; i < file_count; i++)
    {
        if (strcmp("bin/init", initramfs_files[i]->name) == 0)
        {
            init_pos = i;
            init_size = initramfs_files[i]->size;
            break;
        }
    }

    if (init_pos == 0)
        panic("didn't find init in initramfs", 0, 0);
    else
        log(LOG_OK, "Found init file (%d B)", init_size);

    void *init_data = initramfs_files[init_pos]->content;

    init_heap_current = kmalloc(65536);
    int frames = (init_size + PAGE_SIZE - 1) >> 12;

    task_t *init_task = task_create(init_data, 0, "bin/init", 0, 0, frames);
    if (init_task == NULL)
        panic("Failed to create init process", 0, 0);

    log(LOG_OK, "Created init process");
}
