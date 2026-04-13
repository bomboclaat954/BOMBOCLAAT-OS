#include <bfs.h>
#include <api.h>
#include <disk.h>
#include <ram.h>
#include <rand.h>

int check_bfs(int slave) {}

void format_bfs(uint32_t total_sectors, char label[8], int erase)
{
    if (erase)
    {
        for (int i = 0; i < total_sectors; i++)
        {
            ata_erase_sector(i);
        }
    }
    else
        ata_erase_sector(0);

    uint16_t sector[256] = {0};
    uint8_t *b = (uint8_t *)sector;

    memcpy(b + 0, "BOMBOCLAAT", 10);

    write_u8(b + 10, 1);              // fs ver
    write_u16(b + 11, 512);           // bytes per sector
    write_u32(b + 13, total_sectors); // total sectors
    write_u8(b + 17, 2);              // sectors per cluster

    char uuid[8];
    random_seed();
    for (int i = 0; i < 8; i++)
        uuid[i] = letters_digits[rand() % ARRAY_SIZE(letters_digits)];

    memcpy(b + 18, label, 8); // disk label
    memcpy(b + 26, uuid, 8);  // disk unique ID

    for (int i = 0; i < 463; i++)
    {
        write_u8(b + i + 34, 0x00);
    }

    ata_write_sector(0, sector);
}
