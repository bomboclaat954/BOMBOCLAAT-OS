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

#include <fs/ramfs.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <lib/string.h>
#include <bomboclaat/kprintf.h>

ramfs_t *ramfs;
int dir_count = 0;
int files_count = 0;

ramfs_dir_t *ramfs_create_dir(char *name, ramfs_dir_t *parent)
{
    ramfs_dir_t *new_dir = (ramfs_dir_t *)kmalloc(sizeof(ramfs_dir_t));
    if (!new_dir)
        return NULL;

    new_dir->name = name;
    new_dir->parent_dir = parent;
    new_dir->child_dirs_count = 0;
    new_dir->child_dirs = kmalloc(sizeof(ramfs_dir_t *) * 128);
    new_dir->files = kmalloc(sizeof(ramfs_file_t *) * 1024);
    new_dir->files_count = 0;

    if (parent != NULL)
    {
        parent->child_dirs[parent->child_dirs_count] = new_dir;
        parent->child_dirs_count++;
    }

    ramfs->dirs[dir_count] = new_dir;
    dir_count++;

    return new_dir;
}

void print_dir_path(ramfs_dir_t *dir)
{
    if (dir == NULL)
        return;
    if (dir->parent_dir == NULL)
    {
        kprintf("/");
        return;
    }
    print_dir_path(dir->parent_dir);
    if (dir->parent_dir->parent_dir != NULL)
        kprintf("/");
    kprintf("%s", dir->name);
}

void print_dir_files(ramfs_dir_t *dir)
{
    if (dir->files_count > 0)
    {
        for (int i = 0; i < dir->files_count; i++)
            kprintf("   %s  %d B", dir->files[i]->name, dir->files[i]->size);
    }
}

void ramfs_ls()
{
    for (int i = 0; i < dir_count; i++)
    {
        ramfs_dir_t *current = ramfs->dirs[i];
        if (current == NULL)
            continue;
        print_dir_path(current);
        kprintf("\n");
        print_dir_files(current);
        kprintf("\n");
    }
}

ramfs_file_t *ramfs_create_file(ramfs_dir_t *dir, uint8_t *data, int data_size, char *name)
{
    ramfs_file_t *new_file = (ramfs_file_t *)kmalloc(sizeof(ramfs_file_t));
    new_file->dir = dir;
    new_file->content = data;
    new_file->name = name;
    new_file->size = data_size;

    dir->files[dir->files_count] = new_file;
    dir->files_count++;

    return new_file;
}

ramfs_t *init_ramfs()
{
    ramfs = (ramfs_t *)kmalloc(sizeof(ramfs_t));
    ramfs->dirs = kmalloc(sizeof(ramfs_dir_t *) * 128);
    ramfs->files = kmalloc(sizeof(ramfs_file_t *) * 1024);
    if (!ramfs || !ramfs->dirs || !ramfs->files)
        return 0;

    ramfs_dir_t *root = ramfs_create_dir("/", NULL);
    ramfs_dir_t *etc = ramfs_create_dir("etc", root);
    ramfs_dir_t *tmp = ramfs_create_dir("tmp", root);
    ramfs_dir_t *proc = ramfs_create_dir("proc", root);

    return ramfs;
}
