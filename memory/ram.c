/*
    BOMBOCLAAT-OS RAM MANAGEMENT
*/
#include <memory/ram.h>
#include <lib/string.h>
#include <bomboclaat-os/api.h>

uint32_t *bitmap;
uint32_t total_frames;
uint32_t used_frames;

void bitmap_set(uint32_t frame)
{
    bitmap[frame / 32] |= (1 << (frame % 32));
}

void bitmap_unset(uint32_t frame)
{
    bitmap[frame / 32] &= ~(1 << (frame % 32));
}

uint32_t get_free_ram_kb()
{
    return (total_frames - used_frames) * 4;
}

uint32_t get_used_ram_kb()
{
    return used_frames * 4;
}

uint64_t multiboot_get_ram(multiboot_info_t *mbi, int unit) // 0 - B, 1 - kB, 2 - MB
{
    uint64_t total_ram = 0;

    if (!(mbi->flags & (1 << 6)))
        return 0;

    uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

    multiboot_memory_map_t *mmap =
        (multiboot_memory_map_t *)mbi->mmap_addr;

    while ((uint32_t)mmap < mmap_end)
    {
        if (mmap->type == 1)
            total_ram += mmap->len;

        mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    if (unit == 0 || unit > 2)
        return total_ram;
    else if (unit == 1)
        return total_ram / 1024;
    else if (unit == 2)
        return total_ram / (1024 * 1024);
}

void pmm_init(multiboot_info_t *mbi)
{
    bitmap = (uint32_t *)&_kernel_end;

    uint64_t mem_size = multiboot_get_ram(mbi, 0);
    total_frames = mem_size / PAGE_SIZE;
    used_frames = total_frames;

    memset(bitmap, 0xFF, (total_frames / 8));

    if (!(mbi->flags & (1 << 6)))
        panic("no memory map from bootloader", NULL, 0);

    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
    uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

    while ((uint32_t)mmap < mmap_end)
    {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            for (uint64_t addr = mmap->addr; addr < mmap->addr + mmap->len; addr += PAGE_SIZE)
            {
                bitmap_unset(addr / PAGE_SIZE);
                used_frames--;
            }
        }
        mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    uint32_t kernel_start_frame = (uint32_t)&_kernel_start / PAGE_SIZE;
    uint32_t kernel_end_frame = ((uint32_t)bitmap + (total_frames / 8)) / PAGE_SIZE;

    for (uint32_t f = kernel_start_frame; f <= kernel_end_frame; f++)
    {
        bitmap_set(f);
        used_frames++;
    }
}

void memcpy(uint8_t *dst, const char *src, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        dst[i] = src[i];
}

void write_u64(uint8_t *dst, uint64_t val)
{
    dst[0] = val & 0xFF;
    dst[1] = (val >> 8) & 0xFF;
    dst[2] = (val >> 16) & 0xFF;
    dst[3] = (val >> 24) & 0xFF;
    dst[3] = (val >> 32) & 0xFF;
    dst[3] = (val >> 40) & 0xFF;
    dst[3] = (val >> 48) & 0xFF;
    dst[3] = (val >> 56) & 0xFF;
}

void write_u32(uint8_t *dst, uint32_t val)
{
    dst[0] = val & 0xFF;
    dst[1] = (val >> 8) & 0xFF;
    dst[2] = (val >> 16) & 0xFF;
    dst[3] = (val >> 24) & 0xFF;
}

void write_u16(uint8_t *dst, uint16_t val)
{
    dst[0] = val & 0xFF;
    dst[1] = (val >> 8) & 0xFF;
}

void write_u8(uint8_t *dst, uint8_t val)
{
    dst[0] = val & 0xFF;
}
