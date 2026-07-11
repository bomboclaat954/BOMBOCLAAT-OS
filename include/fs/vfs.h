/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#define VFS_MODE_DIR 0
#define VFS_MODE_FILE 1
#define VFS_MODE_DEV 2
#define MAX_FILESYSTEMS 16

struct vfs_inode;
struct device;
struct vfs_inode_ops
{
    int64_t (*read)(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
    int64_t (*write)(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
    struct vfs_inode *(*lookup)(struct vfs_inode *parent, char *name);
    struct vfs_inode *(*mkfile)(struct vfs_inode *parent, char *name);
    struct vfs_inode *(*mkdir)(struct vfs_inode *parent, char *name);
};

struct filesystem
{
    char *name;
    struct vfs_inode *(*mount)(struct device *dev, void *flags);
    struct filesystem *next;
} typedef filesystem_t;

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
    struct vfs_inode *mounted_inode;
    struct vfs_dentry *parent;
} typedef vfs_dentry_t;

struct vfs_mount_s
{
    struct vfs_dentry *mountpoint;
    struct vfs_inode *root_inode;
    struct vfs_mount *next;
} typedef vfs_mount_t;

struct vfs_file
{
    struct vfs_inode *inode;
    uint64_t offset;
    uint32_t flags;
    uint32_t ref_count;
} typedef vfs_file_t;

extern vfs_dentry_t *vfs_root_dentry;

int vfs_setup_inode(vfs_inode_t *inode);
int vfs_read(int fd, void *buf, uint64_t size);
int vfs_write(int fd, void *buf, uint64_t size);
vfs_inode_t *vfs_mkfile(vfs_inode_t *parent, char *name);
vfs_inode_t *vfs_mkdir(vfs_inode_t *parent, char *name);
int vfs_open(char *path, int flags, uint64_t *size_buf);
int vfs_close(int fd);
int parse_path(char *path, char *out_buf[]);
vfs_dentry_t *vfs_find(char *path);
int vfs_mount(void *dev, char *target, char *fs_type, void *flags);
void vfs_register_fs(filesystem_t *fs);
void vfs_init();

#endif
