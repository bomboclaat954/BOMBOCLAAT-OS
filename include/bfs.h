#ifndef BFS_H
#define BFS_H
// BOMBOCLAAT FILE SYSTEM
#include <stdint.h>

typedef struct
{
    char filesystem[10];
    uint8_t fs_ver;
    uint16_t bytes_per_sector;
    uint32_t total_sectors;
    uint8_t sectors_per_cluster;
    char disk_label[8];
    char uuid[8];
    uint8_t zeros[464];
} __attribute__((packed)) bfs_t;

int check_bfs(int slave);
void format_bfs(uint32_t total_sectors, char label[8], int erase);

#endif
