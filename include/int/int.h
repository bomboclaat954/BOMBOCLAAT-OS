/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
// Everything that's connected with interrupts
#ifndef INT_H
#define INT_H

#include <stdint.h>

#define IDT_MAX_DESCRIPTORS 256
#define PIT_CHANNEL0 0x40
#define PIT_CMD 0x43
#define PIT_FREQUENCY 1193182
#define PIT_HZ 1000
#define IA32_APIC_BASE_MSR 0x1B
#define LAPIC_SVR 0x0F0
#define LAPIC_SVR_ENABLE 0x100
#define LAPIC_TIMER 0x320
#define LAPIC_TDCR 0x3E0
#define LAPIC_TICR 0x380
#define TIMER_PERIODIC 0x20000
#define LAPIC_EOI 0xB0
#define LAPIC_TCCR 0x390

typedef struct
{
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

typedef struct
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

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
} __attribute__((packed)) gdtEntries;

extern gdtEntries gdt;
extern gdt_ptr gdtr;
extern tss_ptr tss;

extern idtr_t idtr;
extern int vectors[IDT_MAX_DESCRIPTORS];
extern void *isr_stub_table[];

#ifdef __cplusplus
extern "C"
{
#endif
    void exception_handler(registers_t *r);
    void irq_handler(registers_t *r);
    void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);
    void idt_init(void);
    void pic_disable(void);
    uint64_t pit_get_ticks(void);
    void delay_ms(uint64_t ms);
    void syscall_send(uint64_t nsyscall, const char *args);
    void gdt_tss_init(void);

    void lapic_init();
    void lapic_timer_init(uint32_t count);
    void apic_eoi();
    void lapic_timer_calibrate();
    void ioapic_set_irq(uint8_t gsi, uint8_t vector, uint8_t target_cpu_apic_id);
#ifdef __cplusplus
}
#endif

#endif
