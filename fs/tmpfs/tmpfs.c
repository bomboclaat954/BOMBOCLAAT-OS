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

#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/kprintf.h>
#include <lib/string.h>

struct vfs_inode_ops *tmpfs_inode_ops;

vfs_inode_t *tmpfs_lookup(vfs_inode_t *parent, char *name)
{
    tmpfs_dir_t *dir = (tmpfs_dir_t *)parent->private_data;
    for (int i = 0; i < dir->files_count; i++)
    {
        tmpfs_file_t *file = dir->files[i];
        if (strcmp(file->name, name) == 0)
        {
            vfs_inode_t *ret = (vfs_inode_t *)kmalloc(sizeof(vfs_inode_t));
            ret->id = file->size + 100;
            ret->mode = 0;
            ret->size = file->size;
            ret->private_data = (void *)file;
            ret->ops = tmpfs_inode_ops;

            return ret;
        }
    }
    return NULL;
}

int64_t tmpfs_mkdir(struct vfs_inode *parent, char *name, uint16_t mode)
{
    tmpfs_dir_t new = {
        .files = NULL,
        .files_count = 0,
        .name = name,
        .parent_dir = NULL,
    };

    parent->private_data = &new;
    parent->mode = mode;
    // todo: add the rest of the fields

    if ((tmpfs_dir_t *)parent->private_data == &new)
        return 1;
    else
        return 0;
}

int64_t tmpfs_read(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset)
{
    tmpfs_file_t *file = (tmpfs_file_t *)inode->private_data;
    memcpy((uint8_t *)buffer, file->content + offset, size);
    return size;
}

int64_t tmpfs_write(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset)
{
    tmpfs_file_t *file = (tmpfs_file_t *)inode->private_data;
    memcpy(file->content + offset, buffer, size);
    file->size = file->size + size;
    inode->private_data = file;

    if (offset + size > file->size)
        file->size = offset + size;

    return size;
}

int64_t tmpfs_mkfile(struct vfs_inode *parent, char *name, uint16_t mode)
{
    tmpfs_dir_t *dir = (tmpfs_dir_t *)parent->private_data;
    tmpfs_file_t *new = (tmpfs_file_t *)kmalloc(sizeof(tmpfs_file_t));
    if (!new)
        panic("TMPFS: kmalloc error", 0, 0);
    new->content = (uint8_t *)kmalloc(1024);
    memset(new->content, 0, 1024);
    new->dir = dir;
    new->name = name;
    new->size = 0;

    dir->files[dir->files_count] = new;
    dir->files_count++;

    if (dir->files[dir->files_count])
        return 1;
    else
        return 0;
}

void tmpfs_init()
{
    tmpfs_inode_ops = (struct vfs_inode_ops *)kmalloc(sizeof(struct vfs_inode_ops));
    tmpfs_inode_ops->lookup = tmpfs_lookup;
    tmpfs_inode_ops->mkdir = tmpfs_mkdir;
    tmpfs_inode_ops->mkfile = tmpfs_mkfile;
    tmpfs_inode_ops->read = tmpfs_read;
    tmpfs_inode_ops->write = tmpfs_write;
}
