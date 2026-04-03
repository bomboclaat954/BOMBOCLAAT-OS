#ifndef RAM_H
#define RAM_H
#define PAGE_SIZE 4096
#define MULTIBOOT_MEMORY_AVAILABLE 1
#include <stdint.h>

typedef struct multiboot_info
{
    uint32_t flags;

    uint32_t mem_lower;
    uint32_t mem_upper;

    uint32_t boot_device;
    uint32_t cmdline;

    uint32_t mods_count;
    uint32_t mods_addr;

    uint32_t syms[4];

    uint32_t mmap_length;
    uint32_t mmap_addr;

    uint32_t drives_length;
    uint32_t drives_addr;

    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;

    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

} multiboot_info_t;

typedef struct multiboot_memory_map
{
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} multiboot_memory_map_t;

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

void bitmap_set(uint32_t frame);
void bitmap_unset(uint32_t frame);
uint32_t get_free_ram_kb();
uint32_t get_used_ram_kb();
uint64_t multiboot_get_ram(multiboot_info_t *mbi, int unit);
void pmm_init(multiboot_info_t *mbi);

#endif
