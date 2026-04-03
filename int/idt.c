#include <int.h>

__attribute__((aligned(0x10))) static idt_entry_t idt[256];

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags)
{
    idt_entry_t *descriptor = &idt[vector];

    descriptor->isr_low = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs = 0x08;
    descriptor->attributes = flags;
    descriptor->isr_high = (uint32_t)isr >> 16;
    descriptor->reserved = 0;
}

void idt_init(void)
{
    extern void isr_stub_default(void);

    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = sizeof(idt_entry_t) * 256 - 1;

    for (int i = 0; i < 256; i++)
        idt_set_descriptor(i, (void *)isr_stub_default, 0x8E);

    for (uint8_t i = 0; i < 32; i++)
        idt_set_descriptor(i, isr_stub_table[i], 0x8E);

    pic_remap();

    idt_set_descriptor(32, isr_stub_table[32], 0x8E);

    __asm__ volatile("lidt %0" : : "m"(idtr));
    __asm__ volatile("sti");
}
