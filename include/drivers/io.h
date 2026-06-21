/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IO_H
#define IO_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

    void outb(unsigned short port, unsigned char val);
    unsigned char inb(unsigned short port);
    void outw(unsigned short port, unsigned short val);
    uint16_t inw(unsigned short port);
    unsigned char read_cmos(unsigned char reg);

#ifdef __cplusplus
}
#endif

#endif
