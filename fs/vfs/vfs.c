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
// ! flags are ignored now but I'll fix it later
#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <bomboclaat/kprintf.h>
#include <tasks/tasks.h>

vfs_dentry_t *vfs_root_dentry = NULL;
vfs_inode_t *root_inode = NULL;
int next_id = 1;

int vfs_setup_inode(vfs_inode_t *inode)
{
    inode->id = next_id;
    inode->lookup = NULL;
    inode->mkdir = NULL;
    inode->mkfile = NULL;
    inode->mode = 0;
    inode->read = NULL;
    inode->size = 0;
    inode->write = NULL;

    next_id++;
    if (inode)
        return 1;
    else
        return 0;
}

int vfs_read(int fd, void *buf, uint64_t size)
{
    extern task_t *current_task;
    if (current_task == NULL)
        return -1;
    if (fd < 0 || fd >= MAX_FILES_PER_TASK)
        return -1;

    vfs_file_t *file = current_task->fd_table[fd];
    if (file == NULL)
        return -1;

    vfs_inode_t *inode = file->inode;
    if (!inode->read)
        return -1;

    int bytes_read = inode->read(inode, buf, size, file->offset);
    if (bytes_read > 0)
        file->offset += bytes_read;

    return bytes_read;
}

int vfs_write(int fd, void *buf, uint64_t size)
{
    extern task_t *current_task;
    if (current_task == NULL)
        return -1;
    if (fd < 0 || fd >= MAX_FILES_PER_TASK)
        return -1;

    vfs_file_t *file = current_task->fd_table[fd];
    if (file == NULL)
        return -1;

    vfs_inode_t *inode = file->inode;
    if (!inode->write)
        return -1;

    int bytes_written = inode->write(inode, buf, size, file->offset);
    if (bytes_written > 0)
        file->offset += bytes_written;
        
    return bytes_written;
}

int vfs_mkfile(vfs_inode_t *parent, char *name, uint16_t mode)
{
    if (parent->mkfile(parent, name, mode))
        return 1;
    else
        return 0;
}

int vfs_open(char *path, int flags)
{
    extern task_t *current_task;
    if (current_task == NULL)
        return -1;

    int fd = -1;
    for (int i = 0; i < MAX_FILES_PER_TASK; i++)
    {
        if (current_task->fd_table[i] == NULL)
        {
            fd = i;
            break;
        }
    }

    if (fd == -1)
        return -1;

    if (!vfs_root_dentry->inode->lookup)
        return -1;
    vfs_inode_t *inode = vfs_root_dentry->inode->lookup(vfs_root_dentry->inode, path);
    if (inode == NULL)
        return -1;

    vfs_file_t *file = (vfs_file_t *)kmalloc(sizeof(vfs_file_t));
    if (!file)
        return -1;

    file->inode = inode;
    file->flags = flags;
    file->offset = 0;
    file->ref_count = 1;

    current_task->fd_table[fd] = file;
    return fd;
}

int vfs_close(int fd)
{
    extern task_t *current_task;
    if (current_task == NULL)
        return -1;
    if (fd < 0 || fd >= MAX_FILES_PER_TASK)
        return -1;

    vfs_file_t *file = current_task->fd_table[fd];
    if (file == NULL)
        return -1;

    current_task->fd_table[fd] = NULL;

    file->ref_count--;
    if (file->ref_count == 0)
        kfree(file);

    return 0;
}

void vfs_init()
{
    root_inode = (vfs_inode_t *)kmalloc(sizeof(vfs_inode_t));
    vfs_setup_inode(root_inode);
    if (!root_inode)
        panic("VFS: kmalloc error", 0, 0);

    root_inode->id = 1;
    root_inode->mode = 0755;
    root_inode->size = 0;

    vfs_root_dentry = (vfs_dentry_t *)kmalloc(sizeof(vfs_dentry_t));
    if (!vfs_root_dentry)
        panic("VFS: kmalloc error", 0, 0);

    vfs_root_dentry->name = "/";
    vfs_root_dentry->inode = root_inode;
    vfs_root_dentry->parent = vfs_root_dentry;
}
