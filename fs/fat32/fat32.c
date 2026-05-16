/*
    BOMBOCLAAT-OS FAT32 IMPLEMENTATION
*/
#include <fs/fat32.h>
#include <fs/bpb.h>
#include <memory/kmalloc.h>
#include <memory/ram.h>
#include <drivers/disk.h>
#include <drivers/screen.h>
#include <bomboclaat-os/api.h>
#include <bomboclaat-os/kprintf.h>
#include <lib/string.h>

int data_start = 0;
int root_lba = 0;
bpb_t *bpb = 0;
uint32_t curr_dir_clus = 0;

uint32_t get_root_clus()
{
    return bpb->root_cluster;
}

int cluster_to_lba(int cluster)
{
    return data_start + ((cluster - 2) * bpb->sectors_per_cluster);
}

uint32_t get_next_cluster(uint32_t current_cluster)
{
    uint32_t fat_offset = current_cluster * 4;
    uint32_t fat_sector = bpb->reserved_sectors + (fat_offset / bpb->bytes_per_sector);
    uint32_t entry_offset = fat_offset % bpb->bytes_per_sector;

    uint16_t sector_buffer[256];
    ata_read_sector(fat_sector, sector_buffer);

    uint32_t *fat_table = (uint32_t *)sector_buffer;
    uint32_t next_cluster = fat_table[entry_offset / 4];

    next_cluster &= 0x0FFFFFFF;

    return next_cluster; // if next_cluster >= 0x0FFFFFF8, it's the EOF
}

void format_to_83(const char *input, char *output)
{
    if (strcmp(input, "..") == 0)
    {
        strcpy("..         ", output);
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
    else if (attr == ATTR_HIDDEN)
        return "[ HIDDEN ]";
    else if (attr == ATTR_SYSTEM)
        return "[ SYSTEM ]";
    else if (attr == ATTR_VOLUME_ID)
        return "[ VOLUME ID ]";
    else if (attr == ATTR_DIRECTORY)
        return "[ DIRECTORY ]";
    else if (attr == ATTR_ARCHIVE)
        return "[ FILE ]";
    else
        return "[ ??? ]";
}

void lsdir_cluster(uint32_t dir_cluster)
{
    if (dir_cluster == 0)
        dir_cluster = curr_dir_clus;
    uint32_t current_cluster = dir_cluster;
    uint16_t sector_buffer[256];

    while (current_cluster < 0x0FFFFFF8)
    {
        uint32_t sector_lba = cluster_to_lba(current_cluster);

        for (uint8_t s = 0; s < bpb->sectors_per_cluster; s++)
        {
            ata_read_sector(sector_lba + s, sector_buffer);
            dir_entry_t *entry = (dir_entry_t *)sector_buffer;

            for (int i = 0; i < (bpb->bytes_per_sector / sizeof(dir_entry_t)); i++)
            {
                if (entry[i].name[0] == 0x00)
                    return;

                if ((unsigned char)entry[i].name[0] == 0xE5)
                    continue;

                if (entry[i].attr == ATTR_LFN)
                    continue;

                if (entry[i].attr == ATTR_DIRECTORY)
                {
                    char name_buf[12];
                    memcpy(name_buf, entry[i].name, 11);
                    name_buf[11] = '\0';
                    char *attr = attr_to_txt(entry[i].attr);
                    kprintf("%s       %s\n", name_buf, attr);
                }
                else
                {
                    char name_buf[12];
                    memcpy(name_buf, entry[i].name, 11);
                    name_buf[11] = '\0';
                    char name_formatted[12];
                    format_from_83(name_buf, name_formatted);
                    char *attr = attr_to_txt(entry[i].attr); // useless but looks better that way
                    kprintf("%s   %dB   %s\n", name_formatted, entry[i].size, attr);
                }
            }
        }
        current_cluster = get_next_cluster(current_cluster);
    }
}

dir_entry_t *find(const char *name, uint32_t dir_cluster, uint8_t attr)
{
    char fat_name[11];
    format_to_83(name, fat_name);

    uint32_t current_cluster = dir_cluster;
    static dir_entry_t found_entry;
    uint16_t buf[256];

    while (current_cluster < 0x0FFFFFF8)
    {
        uint32_t lba = cluster_to_lba(current_cluster);
        for (int s = 0; s < bpb->sectors_per_cluster; s++)
        {
            ata_read_sector(lba + s, buf);
            dir_entry_t *entries = (dir_entry_t *)buf;

            for (int i = 0; i < (bpb->bytes_per_sector / sizeof(dir_entry_t)); i++)
            {
                if (entries[i].name[0] == 0x00)
                    return 0;
                if (memcmp(entries[i].name, fat_name, 11) == 0)
                {
                    found_entry = entries[i];
                    return &found_entry;
                }
            }
        }
        current_cluster = get_next_cluster(current_cluster);
    }
    return 0;
}

uint32_t chdir(const char *name, uint32_t current_dir) // ik the names are a bit fucked up but ignore it
{
    dir_entry_t *dir = find(name, current_dir, ATTR_DIRECTORY);
    if (dir->attr == ATTR_DIRECTORY)
    {
        uint32_t new_clus = (dir->cluster_hi << 16) | dir->cluster_lo;
        if (new_clus == 0)
        {
            curr_dir_clus = bpb->root_cluster;
            return curr_dir_clus;
        }
        curr_dir_clus = new_clus;
        return ((dir->cluster_hi << 16) | dir->cluster_lo);
    }
    else
        return current_dir;
}

void read_file_content(dir_entry_t *file, void *output_buffer)
{
    uint32_t current_cluster = ((uint32_t)file->cluster_hi << 16) | file->cluster_lo;
    uint32_t bytes_to_read = file->size;
    uint8_t *out = (uint8_t *)output_buffer;
    uint16_t sector_buf[256];

    while (bytes_to_read > 0 && current_cluster < 0x0FFFFFF8)
    {
        uint32_t lba = cluster_to_lba(current_cluster);

        for (int s = 0; s < bpb->sectors_per_cluster && bytes_to_read > 0; s++)
        {
            ata_read_sector(lba + s, sector_buf);

            uint32_t chunk = (bytes_to_read > 512) ? 512 : bytes_to_read;
            memcpy(out, (char *)sector_buf, chunk);

            out += chunk;
            bytes_to_read -= chunk;
        }
        current_cluster = get_next_cluster(current_cluster);
    }
}

void read(char *name)
{
    dir_entry_t *file = find(name, curr_dir_clus, ATTR_ARCHIVE);

    if (!file)
    {
        puts("File not found", 1);
        return;
    }

    char *buf = kmalloc(file->size + 1);
    if (!buf)
        return;

    read_file_content(file, buf);
    buf[file->size] = '\0';
    kprintf("%s", buf);
    kfree(buf);
}

uint32_t init_fat32()
{
    bpb = (bpb_t *)kmalloc(512);
    if (bpb == 0)
        panic("memory error while initializing FAT32", 0, 0);

    ata_read_sector(0, (uint16_t *)bpb);
    data_start = bpb->reserved_sectors + (bpb->num_FATs * bpb->FAT_size);
    root_lba = data_start + (bpb->root_cluster - 2) * bpb->sectors_per_cluster;
    curr_dir_clus = bpb->root_cluster;
    char fs[9];
    memcpy(fs, bpb->filesystem, 8);
    fs[8] = '\0';
    if (strcmp(fs, "FAT32   ") == 0 && bpb->boot_signature == 0xAA55)
        return curr_dir_clus;
    else
        return 0;
}
