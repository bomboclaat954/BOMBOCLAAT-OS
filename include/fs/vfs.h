/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VFS_H
#define VFS_H

#include <stdint.h>

struct vfs_inode;

struct vfs_inode_ops
{
    int64_t (*read)(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
    int64_t (*write)(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
    struct vfs_inode *(*lookup)(struct vfs_inode *parent, char *name);
    int64_t (*mkfile)(struct vfs_inode *parent, char *name, uint16_t mode);
    int64_t (*mkdir)(struct vfs_inode *parent, char *name, uint16_t mode);
};

struct vfs_inode
{
    uint64_t id;
    uint64_t size;
    uint32_t mode;

    struct vfs_inode_ops *ops;

    void *private_data; // you can save other structures here such as tmpfs_file_t
} typedef vfs_inode_t;

struct vfs_dentry
{
    char *name;
    struct vfs_inode *inode;
    struct vfs_dentry *parent;
} typedef vfs_dentry_t;

struct vfs_file
{
    struct vfs_inode *inode;
    uint64_t offset;
    uint32_t flags;
    uint32_t ref_count;
} typedef vfs_file_t;

int vfs_read(int fd, void *buf, uint64_t size);
int vfs_write(int fd, void *buf, uint64_t size);
int vfs_mkfile(vfs_inode_t *parent, char *name, uint16_t mode);
int vfs_open(char *path, int flags, uint64_t *size_buf);
int vfs_close(int fd);
void vfs_init();

#endif
