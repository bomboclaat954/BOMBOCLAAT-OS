/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DISK_H
#define DISK_H
#include <stdint.h>

#define ATA_PRIMARY_DATA 0x1F0
#define ATA_PRIMARY_COMMAND 0x1F7
#define ATA_CMD_IDENTIFY 0xEC

uint8_t detect_ata_drive();
void get_ata_drive_model(char *buffer);
void ata_read_sector(uint32_t lba, uint16_t *buf);
void ata_write_sector(uint32_t lba, uint16_t *buf);
void ata_erase_sector(uint32_t lba);
uint32_t get_ata_capacity_sectors();

#endif
