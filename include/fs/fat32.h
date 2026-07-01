/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FAT32_H
#define FAT32_H

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LFN 0x0F

#include <stdint.h>
#include <fs/vfs.h>

struct fat32_entry
{
    char name[11];
    uint8_t attr;
    uint8_t nt_res;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t cluster_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t cluster_lo;
    uint32_t size;
} __attribute__((packed)) typedef fat32_entry_t;

extern struct vfs_inode_ops fat32_inode_ops;

vfs_inode_t *fat32_lookup(vfs_inode_t *parent, char *name);
int64_t fat32_read(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
int64_t fat32_write(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset);
int64_t fat32_mkdir(struct vfs_inode *parent, char *name, uint16_t mode);
int64_t fat32_mkfile(struct vfs_inode *parent, char *name, uint16_t mode);
void fat32_init();

#endif
