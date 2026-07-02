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
// I thought scheduler was the worst part of writing a kernel but I underestimated VFS...
// How the fuck Torvalds managed to write it and make it work?
#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <bomboclaat/kprintf.h>
#include <tasks/tasks.h>
#include <lib/string.h>

vfs_dentry_t *vfs_root_dentry;
vfs_inode_t *root_inode;
filesystem_t *registered_filesystems = NULL;
int next_id = 1;

int vfs_setup_inode(vfs_inode_t *inode)
{
    inode = (vfs_inode_t *)kmalloc(sizeof(vfs_inode_t));
    inode->id = next_id;
    inode->mode = 0;
    inode->size = 0;

    next_id++;
    return 1;
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

    struct vfs_inode *inode = file->inode;
    if (!inode->ops->read)
        return -1;

    int bytes_read = inode->ops->read(inode, buf, size, file->offset);
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

    struct vfs_inode *inode = file->inode;
    if (!inode->ops->write)
        return -1;

    int bytes_written = inode->ops->write(inode, buf, size, file->offset);
    if (bytes_written > 0)
        file->offset += bytes_written;

    return bytes_written;
}

vfs_inode_t *vfs_mkfile(vfs_inode_t *parent, char *name)
{
    return parent->ops->mkfile(parent, name);
}

vfs_inode_t *vfs_mkdir(vfs_inode_t *parent, char *name)
{
    return parent->ops->mkdir(parent, name);
}

int vfs_open(char *path, int flags, uint64_t *size_buf)
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
    if (!root_inode->ops->lookup)
        return -1;
    vfs_inode_t *inode = root_inode->ops->lookup(root_inode, path);
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
    *size_buf = inode->size;
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

vfs_dentry_t *vfs_find(char *path)
{
    // TODO: FIX THIS!!!
    if (!path[0] == '/')
    {
        log(LOG_ERR, "Path has to start with \"/\" (root)");
        return NULL;
    }
}

filesystem_t *vfs_find_fs(char *name)
{
    // TODO: write this
}

int vfs_mount(char *source, char *target, char *fs_type, void *flags, void *data)
{
    filesystem_t *target_fs = vfs_find_fs(fs_type);
    if (!target_fs)
    {
        log(LOG_ERR, "Unknown filesystem type");
        return -1;
    }

    vfs_inode_t *target_inode = target_fs->mount(source, flags);
    if (!target_inode)
    {
        log(LOG_ERR, "Filesystem mount failed");
        return -1;
    }

    if (strcmp(target, "/") == 0)
    {
        root_inode = target_inode;
        vfs_root_dentry = (vfs_dentry_t *)kmalloc(sizeof(vfs_dentry_t));
        vfs_root_dentry->name = "/";
        vfs_root_dentry->inode = target_inode;
        vfs_root_dentry->mounted_inode = NULL;
        vfs_root_dentry->parent = vfs_root_dentry;
    }
    else
    {
        vfs_dentry_t *target_dentry = vfs_find(target);
        if (!target_dentry)
        {
            log(LOG_ERR, "Mount point path not found");
            kfree(target_inode);
            return -1;
        }

        target_dentry->mounted_inode = target_inode;
    }

    return 0;
}

void vfs_register_fs(filesystem_t *fs)
{
    if (!fs)
    {
        log(LOG_ERR, "Incorrect FS structure");
        return;
    }

    filesystem_t *curr = registered_filesystems;
    while (curr != NULL)
    {
        if (strcmp(fs->name, curr->name) == 0)
        {
            log(LOG_ERR, "Filesystem name \"%s\" is already taken", fs->name);
            return;
        }
        curr = curr->next;
    }

    fs->next = registered_filesystems;
    registered_filesystems = fs;
}

void vfs_init()
{
    root_inode = (vfs_inode_t *)kmalloc(sizeof(vfs_inode_t));
    extern tmpfs_dir_t *tmpfs_root;
    vfs_setup_inode(root_inode);
    if (!root_inode)
        panic("VFS: kmalloc error", 0, 0);

    root_inode->id = 1;
    root_inode->mode = VFS_MODE_DIR;
    root_inode->size = 0;
    root_inode->ops = &tmpfs_inode_ops;
    root_inode->private_data = (void *)tmpfs_root;

    vfs_root_dentry = (vfs_dentry_t *)kmalloc(sizeof(vfs_dentry_t));
    if (!vfs_root_dentry)
        panic("VFS: kmalloc error", 0, 0);

    vfs_root_dentry->name = "/";
    vfs_root_dentry->inode = root_inode;
    vfs_root_dentry->parent = vfs_root_dentry;
}
