#ifndef DISK_H
#define DISK_H
#include <stdint.h>

#define ATA_PRIMARY_DATA 0x1F0
#define ATA_PRIMARY_COMMAND 0x1F7
#define ATA_CMD_IDENTIFY 0xEC

uint8_t detect_ata_drive(uint8_t slave);
void get_drive_model(uint8_t slave, char *buffer);
void ata_read_sector(uint32_t lba, uint16_t *buf);
void ata_write_sector(uint32_t lba, uint16_t *buf);
uint32_t get_ata_capacity_sectors(uint8_t slave);

#endif
