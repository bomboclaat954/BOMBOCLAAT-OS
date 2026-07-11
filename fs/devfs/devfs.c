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
// Sometimes I wonder why I still want to continue this project
// and the only thing I can come up with is that it's a punishment for my sins
#include <bomboclaat/globals.h>
#include <memory/kmalloc.h>
#include <lib/string.h>
#include <fs/devfs.h>
#include <fs/vfs.h>

#define MAX_DEVFS_ENTRIES 32

struct devfs_entry
{
    char *name;
    struct vfs_inode *inode;
};

static struct vfs_inode *devfs_lookup(struct vfs_inode *parent, char *name);

filesystem_t devfs = {
    .mount = devfs_mount,
    .name = "devfs",
    .next = NULL,
};

vfs_inode_t *devfs_root_inode = NULL;

static struct devfs_entry devfs_entries[MAX_DEVFS_ENTRIES];
static int devfs_entry_count = 0;

static struct vfs_inode_ops devfs_inode_ops = {
    .read = NULL,
    .write = NULL,
    .lookup = devfs_lookup,
    .mkfile = NULL,
    .mkdir = NULL,
};

void devfs_init()
{
    devfs_root_inode = (vfs_inode_t *)kmalloc(sizeof(vfs_inode_t));
    devfs_root_inode->id = 0;
    devfs_root_inode->mode = VFS_MODE_DIR;
    devfs_root_inode->size = 0;
    devfs_root_inode->ops = &devfs_inode_ops;
    devfs_root_inode->private_data = NULL;

    vfs_register_fs(&devfs);
}

struct vfs_inode *devfs_mount(struct device *dev, void *flags)
{
    vfs_inode_t *dev_inode = (vfs_inode_t *)kmalloc(sizeof(vfs_inode_t));
    struct vfs_inode_ops *ops = (struct vfs_inode_ops *)kmalloc(sizeof(struct vfs_inode_ops));

    ops->read = dev->ops->read;
    ops->write = dev->ops->write;
    ops->lookup = NULL;
    ops->mkfile = NULL;
    ops->mkdir = NULL;

    dev_inode->id = 0;
    dev_inode->mode = VFS_MODE_DEV;
    dev_inode->ops = ops;
    dev_inode->private_data = (void *)dev;
    dev_inode->size = 0;

    return dev_inode;
}

int devfs_register_device(struct device *dev)
{
    if (devfs_entry_count >= MAX_DEVFS_ENTRIES)
        return -1;

    vfs_inode_t *dev_inode = devfs_mount(dev, NULL);
    if (!dev_inode)
        return -1;

    devfs_entries[devfs_entry_count].name = dev->name;
    devfs_entries[devfs_entry_count].inode = dev_inode;
    devfs_entry_count++;

    return 0;
}

static struct vfs_inode *devfs_lookup(struct vfs_inode *parent, char *name)
{
    for (int i = 0; i < devfs_entry_count; i++)
    {
        if (strcmp(devfs_entries[i].name, name) == 0)
            return devfs_entries[i].inode;
    }
    return NULL;
}
