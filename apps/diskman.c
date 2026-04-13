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
    puts("Diskman v0.2", 1);

    if (strcmp(opt, "f") == 0)
    {
        puts("New disk label (max 8 characters): ", 0);
        char label[8];
        input(label, 8);
        uint32_t sectors = get_ata_capacity_sectors();
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
            format_bfs(sectors, label, erase);
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
        if (lba_ > get_ata_capacity_sectors())
        {
            puts("Error: this sector doesn't exist", 1);
            return;
        }
        else
        {
            cls();
            draw_main_screen();

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
        if (lba_ > get_ata_capacity_sectors())
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
    else if (strcmp(opt, "c") == 0)
    {
        uint16_t buf[256];
        ata_read_sector(0, buf);
        bfs_t *sector = (bfs_t *)buf;

        char *fs = sector->filesystem;
        fs[10] = '\0';

        if (strcmp(fs, "BOMBOCLAAT") == 0)
        {
            char *uuid = sector->uuid;
            uuid[8] = '\0';

            char *label = sector->disk_label;
            label[8] = '\0';

            puts("Found BFS on the disk", 1);
            puts("Disk label: ", 0);
            puts(label, 1);
        }
        else
            puts("There's no BFS on the disk", 1);
    }
    else if (strcmp(opt, "h") == 0)
    {
        puts("Available options:", 1);
        puts("  - diskman f: format disk", 1);
        puts("  - diskman r: read sector", 1);
        puts("  - diskman e: erase sector", 1);
        puts("  - diskman c: check if disk is formatted to BFS", 1);
    }
    else
    {
        puts("Error: no option provided or bad option", 1);
        puts("Type diskman h for option list", 1);
    }
}
