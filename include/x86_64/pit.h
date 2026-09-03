/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef PIT_H
#define PIT_H
#include <stdint.h>

#define PIT_CHANNEL0 0x40
#define PIT_CMD 0x43
#define PIT_FREQUENCY 1193182
#define PIT_HZ 1000

uint64_t pit_get_ticks(void);
void delay_ms(uint64_t ms);

#endif
