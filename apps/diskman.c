#include <diskman.h>
#include <disk.h>
#include <screen.h>
#include <ram.h>
#include <string.h>
#include <io.h>
#include <bfs.h>
#include <api.h>

void diskman(char *opt)
{
    cls();
    draw_main_screen();
    puts("Diskman v0.1", 1);
    if (strcmp(opt, "f") == 0)
    {
        puts("New disk label (max 8 characters): ", 0);
        char label[8];
        input(label, 8);
        uint32_t sectors = get_ata_capacity_sectors(0);
        puts("Erase the disk before formatting? [y/n]: ", 0);
        char e[1];
        input(e, 1);
        int erase = 0;
        if (strcmp(e, "y") == 0 || strcmp(e, "Y") == 0)
            erase = 1;
        else
            erase = 0;
        puts("Are you sure? All data will be lost [y/n]: ", 0);
        char a[1];
        input(a, 1);
        if (strcmp(a, "y") == 0 || strcmp(a, "Y") == 0)
        {
            puts("Formatting...", 1);
            format_bfs(sectors, 1, sectors - 1, label, erase);
            puts("Done", 1);
        }
        else
        {
            puts("Abort", 1);
            return;
        }
    }
    else if (strcmp(opt, "r") == 0)
    {
        puts("LBA: ", 0);
        char lba[16];
        input(lba, 16);
        uint32_t lba_ = atoi(lba);
        if (lba_ > get_ata_capacity_sectors(0))
        {
            puts("Error: this sector doesn't exist", 1);
            return;
        }
        else
        {
            uint16_t buf[256];
            ata_read_sector(lba_, buf);
            char *tmp = "";
            for (int i = 0; i < 256; i++)
            {
                tmp = "";
                itoa(reverse_endian(buf[i]), tmp, 16);
                if (strlen(tmp) == 1)
                    tmp = join("000", tmp, tmp, 0);
                else if (strlen(tmp) == 2)
                    tmp = join("00", tmp, tmp, 0);
                else if (strlen(tmp) == 3)
                    tmp = join("0", tmp, tmp, 0);
                puts(tmp, 0);
                puts(" ", 0);
            }
        }
    }
    else if (strcmp(opt, "e") == 0)
    {
        puts("LBA: ", 0);
        char lba[16];
        input(lba, 16);
        uint32_t lba_ = atoi(lba);
        if (lba_ > get_ata_capacity_sectors(0))
        {
            puts("Error: this sector doesn't exist", 1);
            return;
        }
        else
        {
            puts("Erasing...", 1);
            ata_erase_sector(lba_);
            puts("Done", 1);
        }
    }
    else if (strcmp(opt, "h") == 0)
    {
        puts("Available options:", 1);
        puts("  - diskman f: format disk", 1);
        puts("  - diskman r: read sector", 1);
        puts("  - diskman e: erase sector", 1);
    }
    else
    {
        puts("Error: no option provided or bad option", 1);
        puts("Type diskman for option list", 1);
    }
}
