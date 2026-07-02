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
// In Poland we have an interesting unit - "w chuj" (read: v huy) meaning a lot
// There'll be w chuj work.
#include <fs/fat32.h>
#include <fs/bpb.h>
#include <fs/vfs.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <drivers/ata.h>
#include <drivers/screen.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/kprintf.h>
#include <lib/string.h>

int data_start = 0;
int root_lba = 0;
bpb_t *bpb = 0;
uint32_t total_clusters = 0;

struct vfs_inode_ops fat32_inode_ops = {
    .lookup = fat32_lookup,
    .mkdir = fat32_mkdir,
    .mkfile = fat32_mkfile,
    .read = fat32_read,
    .write = fat32_write,
};

int cluster_to_lba(int cluster)
{
    if (cluster == 0)
        cluster = bpb->root_cluster;
    return data_start + ((cluster - 2) * bpb->sectors_per_cluster);
}

uint32_t get_next_cluster(uint32_t current_cluster)
{
    if (current_cluster < 2)
        return 0x0FFFFFF8;

    uint32_t fat_offset = current_cluster * 4;
    uint32_t fat_sector = bpb->reserved_sectors + (fat_offset / bpb->bytes_per_sector);
    uint32_t entry_offset = fat_offset % bpb->bytes_per_sector;

    uint8_t *sector_buffer = kmalloc(512);
    ata_read_sector(fat_sector, (uint16_t *)sector_buffer);

    uint32_t next_cluster = *(uint32_t *)&sector_buffer[entry_offset];
    next_cluster &= 0x0FFFFFFF;

    kfree(sector_buffer);
    return next_cluster;
}

void format_to_83(const char *input, char *output)
{
    if (strcmp(input, "..") == 0)
    {
        memcpy(output, "..         ", 11);
        return;
    }
    for (int i = 0; i < 11; i++)
        output[i] = ' ';

    int i = 0;
    while (input[i] != '.' && input[i] != '\0' && i < 8)
    {
        output[i] = (input[i] >= 'a' && input[i] <= 'z') ? input[i] - 32 : input[i];
        i++;
    }

    const char *dot = strchr(input, '.');
    if (dot)
    {
        dot++;
        for (int j = 0; j < 3 && dot[j] != '\0'; j++)
            output[8 + j] = (dot[j] >= 'a' && dot[j] <= 'z') ? dot[j] - 32 : dot[j];
    }
}

void format_from_83(const char *input, char *output)
{
    char out[12];
    for (int i = 0; i < 12; i++)
    {
        if (input[i] == ' ')
            out[i] = '\t';
        else
            out[i] = input[i];
        if (i == 7)
            out[i] = '.';
    }
    to_lower_case(out);
    memcpy(output, out, 12);
}

char *attr_to_txt(uint8_t attr)
{
    if (attr == ATTR_READ_ONLY)
        return "[ READ-ONLY ]";
    if (attr == ATTR_HIDDEN)
        return "[ HIDDEN ]";
    if (attr == ATTR_SYSTEM)
        return "[ SYSTEM ]";
    if (attr == ATTR_VOLUME_ID)
        return "[ VOLUME ID ]";
    if (attr == ATTR_DIRECTORY)
        return "[ DIRECTORY ]";
    if (attr == ATTR_ARCHIVE)
        return "[ FILE ]";
    return "[ ??? ]";
}

vfs_inode_t *fat32_lookup(vfs_inode_t *parent, char *name)
{
    char fat_name[11];
    format_to_83(name, fat_name);

    fat32_entry_t *dir = (fat32_entry_t *)parent->private_data;

    uint32_t current_cluster = ((uint32_t)dir->cluster_hi << 16) | dir->cluster_lo;
    uint16_t buf[512];

    while (current_cluster < 0x0FFFFFF8 && current_cluster >= 2)
    {
        uint32_t lba = cluster_to_lba(current_cluster);
        for (int s = 0; s < bpb->sectors_per_cluster; s++)
        {
            ata_read_sector(lba + s, (uint16_t *)buf);
            fat32_entry_t *entries = (fat32_entry_t *)buf;

            for (int i = 0; i < (bpb->bytes_per_sector / sizeof(fat32_entry_t)); i++)
            {
                if (entries[i].name[0] == 0x00)
                    return parent;

                if (entries[i].name[0] == 0xE5 || entries[i].attr == 0x08)
                    continue;

                if (memcmp(entries[i].name, fat_name, 11) == 0)
                {
                    vfs_inode_t *new_inode = (vfs_inode_t *)kmalloc(sizeof(vfs_inode_t));
                    new_inode->id = parent->id + 1;
                    new_inode->mode = 0;
                    new_inode->ops = parent->ops;
                    memcpy(new_inode->private_data, (uint8_t *)buf, sizeof(fat32_entry_t));
                    new_inode->size = sizeof(fat32_entry_t);
                    return new_inode;
                }
            }
        }
        current_cluster = get_next_cluster(current_cluster);
    }
    return 0;
}

int64_t fat32_read(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset)
{
    fat32_entry_t *file = (fat32_entry_t *)inode->private_data;
    uint32_t current_cluster = ((uint32_t)file->cluster_hi << 16) | file->cluster_lo;
    uint32_t bytes_to_read = file->size;
    uint8_t *out = (uint8_t *)buffer;
    uint16_t *sector_buf = kmalloc(sizeof(uint16_t) * 512);

    while (bytes_to_read > 0 && current_cluster < 0x0FFFFFF8 && current_cluster >= 2)
    {
        uint32_t lba = cluster_to_lba(current_cluster);

        for (int s = 0; s < bpb->sectors_per_cluster && bytes_to_read > 0; s++)
        {
            ata_read_sector(lba + s, (uint16_t *)sector_buf);

            uint32_t chunk = (bytes_to_read > 512) ? 512 : bytes_to_read;
            memcpy(out, (char *)sector_buf, chunk);

            out += chunk;
            bytes_to_read -= chunk;
        }
        current_cluster = get_next_cluster(current_cluster);
    }
    kfree(sector_buf);
}

int64_t fat32_write(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset)
{
    return 0;
}

struct vfs_inode *fat32_mkdir(struct vfs_inode *parent, char *name)
{
    vfs_inode_t new_inode;
    return &new_inode;
}

struct vfs_inode *fat32_mkfile(struct vfs_inode *parent, char *name)
{
    vfs_inode_t new_inode;
    return &new_inode;
}

uint32_t find_free_cluster(void)
{
    uint16_t *buf = kmalloc(sizeof(uint16_t) * 512);

    for (uint32_t cluster = 2; cluster < total_clusters; cluster++)
    {
        uint32_t byte_offset = cluster * 4;
        uint32_t sector = bpb->reserved_sectors + (byte_offset / bpb->bytes_per_sector);
        uint32_t offset = byte_offset % bpb->bytes_per_sector;

        ata_read_sector(sector, (uint16_t *)buf);

        uint32_t entry = *(uint32_t *)&buf[offset];
        entry &= 0x0FFFFFFF;

        if (entry == 0)
            return cluster;
    }
    kfree(buf);
    return 0;
}

void fat32_init()
{
    /*bpb = (bpb_t *)kmalloc(512);
    if (bpb == 0)
        panic("memory error while initializing FAT32", 0, 0);

    ata_read_sector(0, (uint16_t *)bpb);
    char fs[9];
    memcpy(fs, bpb->filesystem, 8);
    fs[8] = '\0';
    if (strcmp(fs, "FAT32   ") == 0 && bpb->boot_signature == 0xAA55)
    {
        data_start = bpb->reserved_sectors + (bpb->num_FATs * bpb->FAT_size);
        root_lba = data_start + (bpb->root_cluster - 2) * bpb->sectors_per_cluster;
        total_clusters = (bpb->total_sectors - bpb->reserved_sectors - bpb->FAT_size) / (bpb->sectors_per_cluster * bpb->bytes_per_sector);
    }*/
}
