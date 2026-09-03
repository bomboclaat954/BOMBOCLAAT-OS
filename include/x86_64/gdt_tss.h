/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GDT_TSS_H
#define GDT_TSS_H
#include <stdint.h>

typedef struct
{
    uint16_t limit;
    uint16_t baselo;
    uint8_t basemid;
    uint8_t access;
    uint8_t gran;
    uint8_t basehi;
} __attribute__((packed)) gdt_entry;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr;

typedef struct
{
    uint16_t len;
    uint16_t baselo;
    uint8_t basemid;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t basehi;
    uint32_t baseup32;
    uint32_t reserved;
} __attribute__((packed)) tss_entry;

typedef struct
{
    uint32_t unused0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t unused1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t unused2;
    uint32_t iopb;
} __attribute__((packed)) tss_ptr;

typedef struct
{
    gdt_entry descs[11];
    tss_entry tss;
} __attribute__((packed)) gdt_entries;

extern gdt_entries gdt;
extern gdt_ptr gdtr;
extern tss_ptr tss;

void gdt_tss_init(void);

#endif
