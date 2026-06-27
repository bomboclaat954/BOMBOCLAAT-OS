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

#include <int/int.h>
#include <stddef.h>
#include <memory/memtools.h>

gdtEntries gdt = {0};
gdt_ptr gdtr = {0};
tss_ptr tss = {0};

void gdt_load_tss(tss_ptr *tss)
{
    size_t addr = (size_t)tss;
    gdt.tss.baselo = (uint16_t)addr;
    gdt.tss.basemid = (uint8_t)(addr >> 16);
    gdt.tss.flags1 = 0b10001001;
    gdt.tss.flags2 = 0;
    gdt.tss.basehi = (uint8_t)(addr >> 24);
    gdt.tss.baseup32 = (uint32_t)(addr >> 32);
    gdt.tss.reserved = 0;

    asm volatile("ltr %0" ::"rm"((uint16_t)0x58) : "memory");
}

void gdt_reload(void)
{
    asm volatile(
        "lgdt %0\n"
        "push $0x28\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x30, %%rax\n"
        "mov %%rax, %%ds\n"
        "mov %%rax, %%es\n"
        "mov %%rax, %%fs\n"
        "mov %%rax, %%gs\n"
        "mov %%rax, %%ss\n"
        :
        : "m"(gdtr)
        : "rax", "memory");
}

void gdt_tss_init(void)
{
#define ENTRY(i, _l, _bl, _bm, _acc, _gr, _bh) \
    {                                          \
        gdt.descs[i].limit = (_l);             \
        gdt.descs[i].baselo = (_bl);           \
        gdt.descs[i].basemid = (_bm);          \
        gdt.descs[i].access = (_acc);          \
        gdt.descs[i].gran = (_gr);             \
        gdt.descs[i].basehi = (_bh);           \
    }

    ENTRY(0, 0, 0, 0, 0, 0, 0);
    ENTRY(1, 0xffff, 0, 0, 0b10011010, 0b00000000, 0);
    ENTRY(2, 0xffff, 0, 0, 0b10010010, 0b00000000, 0);
    ENTRY(3, 0xffff, 0, 0, 0b10011010, 0b11001111, 0);
    ENTRY(4, 0xffff, 0, 0, 0b10010010, 0b11001111, 0);
    ENTRY(5, 0, 0, 0, 0b10011010, 0b00100000, 0);
    ENTRY(6, 0, 0, 0, 0b10010010, 0b00000000, 0);
    ENTRY(7, 0, 0, 0, 0b11110010, 0b01000000, 0);
    ENTRY(8, 0, 0, 0, 0b11111010, 0b00100000, 0);

#undef ENTRY

    gdt.tss.len = sizeof(tss_ptr) - 1;
    gdt.tss.baselo = 0;
    gdt.tss.basemid = 0;
    gdt.tss.flags1 = 0b10001001;
    gdt.tss.flags2 = 0;
    gdt.tss.basehi = 0;
    gdt.tss.baseup32 = 0;
    gdt.tss.reserved = 0;

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt;

    gdt_reload();
    memset(&tss, 0, sizeof(tss_ptr));
    gdt_load_tss(&tss);
}
