/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <memory/pmm.h>
#include <memory/memtools.h>
#include <lib/string.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/panic.h>

uint8_t *bitmap;
uintptr_t total_frames = 0;
uintptr_t used_frames = 0;
uint64_t mem_size = 0;

extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];

void bitmap_set(uintptr_t frame)
{
    bitmap[frame / 8] |= (1ULL << (frame % 8));
}

void bitmap_unset(uintptr_t frame)
{
    bitmap[frame / 8] &= ~(1ULL << (frame % 8));
}

uintptr_t get_total_frames()
{
    return total_frames;
}

uintptr_t get_free_frames()
{
    return total_frames - used_frames;
}

void *pmm_alloc_frame()
{
    for (uintptr_t i = 1; i < total_frames; i++)
    {
        if ((bitmap[i / 8] & (1 << (i % 8))) == 0)
        {
            bitmap_set(i);
            used_frames++;
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free_frame(void *phys)
{
    if ((uintptr_t)phys % PAGE_SIZE != 0)
        panic("tried to free unaligned frame", 0, 0);
    else if ((uintptr_t)phys > total_frames * PAGE_SIZE)
        panic("tried to free too high address", 0, 0);

    uintptr_t frame = (uintptr_t)phys / PAGE_SIZE;
    uintptr_t byte_index = frame / 8;
    uintptr_t bit_index = frame % 8;
    if ((bitmap[byte_index] & (1 << bit_index)) == 0)
    {
        kprintf("The frame that's requested to be freed is already free\n");
        return;
    }

    bitmap_unset(frame);
    used_frames--;
}

void pmm_init(struct limine_memmap_response *memmap, struct limine_hhdm_response *hhdm)
{
    uint64_t top_address = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->base + entry->length > top_address)
        {
            top_address = entry->base + entry->length;
        }
    }
    total_frames = top_address / PAGE_SIZE;
    uint64_t bitmap_size = total_frames / 8;

    uint64_t bitmap_physical_addr = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size)
        {
            bitmap_physical_addr = (entry->base + entry->length - bitmap_size) & ~(PAGE_SIZE - 1);
            break;
        }
    }
    bitmap = (uint8_t *)(bitmap_physical_addr + hhdm->offset);
    used_frames = total_frames;
    memset(bitmap, 0xFF, (total_frames / 8));
    struct limine_memmap_entry **entries = memmap->entries;
    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            for (uint64_t addr = entry->base; addr < entry->base + entry->length; addr += PAGE_SIZE)
            {
                if (addr >= bitmap_physical_addr && addr < bitmap_physical_addr + bitmap_size)
                    continue;
                bitmap_unset(addr / PAGE_SIZE);
                used_frames--;
            }
        }
    }
}

ram_t init_memmap(struct limine_memmap_response *memmap)
{
    ram_t ret;
    ret.total = 0;
    ret.usable = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        uint64_t base = entry->base;
        uint64_t length = entry->length;
        uint32_t type = entry->type;
        // kprintf("Entry: %d | Base: %x | Length: %x | Type: %d\n", i, base, length, type);
        switch (entry->type)
        {
        case LIMINE_MEMMAP_USABLE:
            ret.usable += entry->length;
            ret.total += entry->length;
            break;

        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        case LIMINE_MEMMAP_RESERVED_MAPPED:
            ret.total += entry->length;
            break;

        default:
            break;
        }
    }
    mem_size = ret.usable;
    return ret;
}
