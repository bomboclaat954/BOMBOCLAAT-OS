#ifndef FAT32_H
#define FAT32_h
#include <stdint.h>

typedef struct
{
    // common FAT offsets
    uint8_t jmp[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t bytes_per_cluster;
    uint16_t reserver_sectors;
    uint8_t num_FATs;
    uint16_t root_dir_entries; // always 0 for FAT32
    uint16_t total_sectors_16; // also always 0
    uint8_t media_type;
    uint16_t sectors_per_fat; // used in FAT12/FAT16, in FAT32 always 0
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors;
    // offsets specific for FAT32
    uint32_t FAT_size;
    uint16_t flags;
    uint16_t fs_ver;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t boot_sec_copy;
    uint32_t reserved[3];
    uint8_t int13_drive_number;
    uint8_t reserved1;
    uint8_t sig1;
    uint32_t volume_id;
    char volume_label[11];
    char filesystem[8];
    uint8_t bootloader[420];
    uint16_t boot_signature; // must be 0xAA55 if drive is bootable
} __attribute__((packed)) fat32_t;

#endif
