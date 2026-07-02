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
// When I was 9 I watched my first C++ lesson. I'm really starting to regret it.
// ngl it's hard to write it sober
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/kprintf.h>
#include <lib/string.h>

tmpfs_dir_t *tmpfs_root;
struct vfs_inode_ops tmpfs_inode_ops = {
    .lookup = tmpfs_lookup,
    .mkdir = tmpfs_mkdir,
    .mkfile = tmpfs_mkfile,
    .read = tmpfs_read,
    .write = tmpfs_write,
};
filesystem_t tmpfs = {
    .mount = tmpfs_mount,
    .name = "tmpfs",
    .next = NULL,
};

vfs_inode_t *tmpfs_mount(void *dev, void *flags)
{
    // TODO: write this and don't get crazy
}

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
            ret->ops = &tmpfs_inode_ops;

            return ret;
        }
    }
    return NULL;
}

vfs_inode_t *tmpfs_mkdir(struct vfs_inode *parent, char *name)
{
    tmpfs_dir_t new = {
        .files = NULL,
        .files_count = 0,
        .name = name,
        .parent_dir = (tmpfs_dir_t *)parent->private_data,
    };

    vfs_inode_t new_inode = {
        .id = 0,
        .mode = VFS_MODE_DIR,
        .ops = &tmpfs_inode_ops,
        .private_data = &new,
        .size = 0,
    };
    return &new_inode;
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

vfs_inode_t *tmpfs_mkfile(struct vfs_inode *parent, char *name)
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

    vfs_inode_t new_inode = {
        .id = 0,
        .mode = VFS_MODE_FILE,
        .ops = &tmpfs_inode_ops,
        .private_data = (void *)new,
        .size = new->size,
    };

    return &new_inode;
}

void tmpfs_init()
{
    tmpfs_root = (tmpfs_dir_t *)kmalloc(sizeof(tmpfs_dir_t));
    tmpfs_root->files_count = 0;
    tmpfs_root->name = "/";
    tmpfs_root->parent_dir = tmpfs_root;

    filesystem_t tmpfs = {
        .mount = NULL,
        .name = "tmpfs",
        .next = NULL,
    };

    vfs_register_fs(&tmpfs);
}
