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
    uint64_t first_partition_start_lba;
    uint64_t first_partition_sectors_num;
    char disk_label[8];
    char uuid[8];
    uint8_t zeros[462];
} __attribute__((packed)) bfs_t;

int check_bfs(int slave);
void format_bfs(uint32_t total_sectors, uint64_t f_p_start, uint64_t f_p_sectors, char label[8], int erase);

#endif
