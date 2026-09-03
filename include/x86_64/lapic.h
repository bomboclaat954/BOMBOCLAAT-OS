/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LAPIC_H
#define LAPIC_H
#include <stdint.h>

#define IA32_APIC_BASE_MSR 0x1B
#define LAPIC_SVR 0x0F0
#define LAPIC_SVR_ENABLE 0x100
#define LAPIC_TIMER 0x320
#define LAPIC_TDCR 0x3E0
#define LAPIC_TICR 0x380
#define TIMER_PERIODIC 0x20000
#define LAPIC_EOI 0xB0
#define LAPIC_TCCR 0x390

void lapic_init();
void lapic_timer_init(uint32_t count);
void apic_eoi();
void lapic_timer_calibrate();
void ioapic_set_irq(uint8_t gsi, uint8_t vector, uint8_t target_cpu_apic_id);

#endif
