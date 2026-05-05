/*
    INTERRUPTS
*/
#ifndef INT_H
#define INT_H
#define IDT_MAX_DESCRIPTORS 256
#include <stdint.h>

typedef struct
{
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t isr_high;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

typedef struct
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
} __attribute__((packed)) registers_t;

static idtr_t idtr;
static int vectors[IDT_MAX_DESCRIPTORS];
extern void *isr_stub_table[];

#ifdef __cplusplus
extern "C"
{
#endif

    void exception_handler(registers_t *r);
    void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);
    void idt_init(void);
    static inline void io_wait(void);
    void pic_remap(void);
    void pit_init(void);
    void pit_tick(void);
    uint32_t pit_get_ticks(void);
    void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif
