/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DEVFS_H
#define DEVFS_H
#include <stdint.h>
#include <memory/stack.h>
#include <fs/vfs.h>

struct dev_ops
{
    int64_t (*read)(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
    int64_t (*write)(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
};

struct device
{
    char *name;
    struct dev_ops *ops;
    stack_t data_stack;
    uint64_t data_stack_size;
} __attribute__((packed)) typedef dev_t;

extern vfs_inode_t *devfs_root_inode;

void devfs_init();
struct vfs_inode *devfs_mount(struct device *dev, void *flags);
int devfs_register_device(struct device *dev);

#endif
