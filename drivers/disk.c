/*
    BOMBOCLAAT-OS DISK SUPPORT
*/
#include <disk.h>
#include <io.h>
#include <api.h>

uint8_t ata_wait_drq(void)
{
    uint8_t status;
    while (1)
    {
        status = inb(ATA_PRIMARY_COMMAND);
        if (status & 0x01)
            return 0;
        if (status & 0x20)
            return 0;
        if (status & 0x80)
            continue;
        if (status & 0x08)
            return 1;
    }
}

uint8_t detect_ata_drive()
{
    outb(0x1F6, 0xE0);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);
    uint8_t status = inb(ATA_PRIMARY_COMMAND);
    if (status == 0)
        return 0;
    while (inb(ATA_PRIMARY_COMMAND) & 0x80)
        ;
    return 1;
}

void ata_wait_ready()
{
    inb(ATA_PRIMARY_COMMAND);
    inb(ATA_PRIMARY_COMMAND);
    inb(ATA_PRIMARY_COMMAND);
    inb(ATA_PRIMARY_COMMAND);

    while (inb(ATA_PRIMARY_COMMAND) & 0x80)
        ;
}

void get_drive_model(char *buffer)
{
    outb(0x1F6, 0xE0);
    outb(0x1F7, ATA_CMD_IDENTIFY);

    uint8_t status = inb(0x1F7);
    if (status == 0)
        return;

    while (inb(0x1F7) & 0x80)
        ;
    while (!(inb(0x1F7) & 0x08))
        ;

    uint16_t data[256];
    for (int i = 0; i < 256; i++)
    {
        data[i] = inw(ATA_PRIMARY_DATA);
    }

    for (int i = 0; i < 20; i++)
    {
        buffer[i * 2] = (char)(data[27 + i] >> 8);
        buffer[i * 2 + 1] = (char)(data[27 + i] & 0xFF);
    }
    buffer[40] = '\0';
}

void ata_read_sector(uint32_t lba, uint16_t *buf)
{
    ata_wait_ready();
    // send sector address
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));

    outb(ATA_PRIMARY_COMMAND, 0x20); // READ

    if (!ata_wait_drq())
        panic("ATA read error", NULL, 0);

    // read data
    for (int i = 0; i < 256; i++)
    {
        buf[i] = inw(0x1F0);
    }
    ata_wait_ready();
}

void ata_write_sector(uint32_t lba, uint16_t *buf)
{
    ata_wait_ready();
    // send sector address
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));

    outb(ATA_PRIMARY_COMMAND, 0x30); // WRITE

    uint8_t status = inb(ATA_PRIMARY_COMMAND);
    while (status & 0x80)
        status = inb(0x1F7);

    if (status & 0x01)
        return;

    while ((inb(0x1F7) & 0x88) != 0x08)
        ;

    // send data
    for (int i = 0; i < 256; i++)
    {
        outw(0x1F0, buf[i]);
    }

    // force write
    outb(ATA_PRIMARY_COMMAND, 0xE7);
    while (inb(ATA_PRIMARY_COMMAND) & 0x80)
        ;
}

void ata_erase_sector(uint32_t lba)
{
    uint16_t buf[256];
    for (int i = 0; i < 256; i++)
    {
        buf[i] = 0x00;
    }
    ata_write_sector(lba, buf);
}

uint32_t get_ata_capacity_sectors()
{
    outb(0x1F6, 0xE0);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);

    while (inb(ATA_PRIMARY_COMMAND) & 0x80)
        ;
    if (!(inb(ATA_PRIMARY_COMMAND) & 0x08))
        return 0;

    uint16_t data[256];
    for (int i = 0; i < 256; i++)
    {
        data[i] = inw(ATA_PRIMARY_DATA);
    }

    uint32_t low = data[60];
    uint32_t high = data[61];

    uint32_t total_sectors = low | (high << 16);

    return total_sectors;
}
