/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TMPFS_H
#define TMPFS_H
#include <stdint.h>
#include <fs/vfs.h>

struct tmpfs_dir
{
    char *name;
    struct tmpfs_dir *parent_dir;
    struct tmpfs_file *files[128];
    int files_count;
} __attribute__((packed)) typedef tmpfs_dir_t;

struct tmpfs_file
{
    char *name;
    tmpfs_dir_t *dir;
    uint64_t size;
    uint8_t *content;
} __attribute__((packed)) typedef tmpfs_file_t;

extern struct vfs_inode_ops *tmpfs_inode_ops;

vfs_inode_t *tmpfs_lookup(vfs_inode_t *parent, char *name);
int64_t tmpfs_mkdir(struct vfs_inode *parent, char *name, uint16_t mode);
int64_t tmpfs_read(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
int64_t tmpfs_write(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
int64_t tmpfs_mkfile(struct vfs_inode *parent, char *name, uint16_t mode);
void tmpfs_init();

#endif
