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
// ! WARNING: you'll find a lot of unnecessary comments in this code. I write them bc I just like sharing my thoughts
/*
    Dear user,
    I really want to thank you for downloading and viweing this code. I'm really happy that you're interested in this.
    If you're a professional OS developer and you see some bugs or errors here, please contact me, I'd really appreciate that.
    Enjoy the code and write a feedback if you want. May God bless you,
    Jakub Fietko, the author.
    (and that's the last decent comment you'll see here)
*/

#include <bomboclaat/kprintf.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/initramfs.h>
#include <bomboclaat/syscall.h>
#include <bomboclaat/logo.h>
#include <drivers/io.h>
#include <drivers/screen.h>
#include <drivers/ata.h>
#include <drivers/serial.h>
#include <drivers/acpi.h>
#include <drivers/keyboard.h>
#include <boot/limine.h>
#include <x86_64/cpu.h>
#include <x86_64/gdt_tss.h>
#include <x86_64/idt.h>
#include <x86_64/lapic.h>
#include <x86_64/timer.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <fs/fat32.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <fs/devfs.h>
#include <tasks/tasks.h>
#include <lib/string.h>
#include <sys/utsname.h>

extern struct utsname utsname;

/*
    About versioning system:
        Pattern: X.Y(.Z)
        X increases when Y is too big to look nice (insead of 7.20 there'll be 8.0)
        Y increases when I add something important that works (the legend says it MAY happen one day)
        Z increases when I fix a mistake, add something less important or add something that doesn't work yet
        You'll also see another number at the end sometimes. It's build number (for example 7.1.2-b34)
*/

stack_t system_stack;
uintptr_t fbf_size = 0;
uintptr_t fbf_phys = 0;

__attribute__((used, section(".limine_requests"))) static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);
__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
};
__attribute__((used, section(".limine_requests"))) volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
};
__attribute__((used, section(".limine_requests"))) volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0,
};
__attribute__((used, section(".limine_requests"))) volatile struct limine_firmware_type_request firmware_request = {
    .id = LIMINE_FIRMWARE_TYPE_REQUEST_ID,
    .revision = 0,
};
__attribute__((used, section(".limine_requests_start"))) static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

void fpu_enable()
{
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2);
    cr0 |= (1 << 1);
    asm volatile("mov %0, %%cr0" ::"r"(cr0));

    asm volatile("fninit");
}

void sse_enable()
{
    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9) | (1 << 10);
    asm volatile("mov %0, %%cr4" ::"r"(cr4));
}

vmm_table_t *kernel_pml4_virt;
uint64_t hhdm_offset;
uint64_t fbf_pitch = 0;
uint64_t fbf_width = 0;
uint64_t fbf_height = 0;

void cpuid(uint32_t leaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    asm volatile("cpuid"
                 : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                 : "a"(leaf));
}

void get_cpu_model(char *buf)
{
    unsigned int a, b, c, d;
    unsigned int *ptr = (unsigned int *)buf;
    for (unsigned int i = 0; i < 3; i++)
    {
        unsigned int function = 0x80000002 + i;
        cpuid(function, &a, &b, &c, &d);
        ptr[i * 4 + 0] = a;
        ptr[i * 4 + 1] = b;
        ptr[i * 4 + 2] = c;
        ptr[i * 4 + 3] = d;
    }
    buf[48] = '\0';
}

void kinit(void)
{
    if (!(LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)))
        asm volatile("hlt");

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
        asm volatile("hlt");

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_hhdm_response *hhdm = hhdm_request.response;
    struct limine_rsdp_response *rsdp = rsdp_request.response;

    init_screen_driver(fb);
    sse_enable();
    fpu_enable();
    sprintf(utsname.version, "BUILD #%d", BUILD_NUMBER);
    for (int i = 0; i < LOGO_HEIGHT; i++)
        kprintf(logo[i]);

    log(LOG_INFO, "Kernel version: %s %s", utsname.release, utsname.version);

    char cpu[49];
    get_cpu_model(cpu);
    log(LOG_INFO, "CPU: %s", cpu);

    if (memmap == NULL || memmap->entry_count == 0)
        panic("unable to get memory map", 0, 0);

    ram_t mem = init_memmap(memmap);
    if (mem.total == 0 || mem.usable == 0)
        panic("error while getting RAM size", 0, 0);
    log(LOG_INFO, "RAM: %d MB", mem.total / (1024 * 1024));

    if (firmware_request.response)
    {
        if (firmware_request.response->firmware_type != LIMINE_FIRMWARE_TYPE_EFI64)
            log(LOG_ERR, "BIOS detected");
    }
    else
        panic("error while getting firmware type", 0, 0);

    idt_init();
    log(LOG_OK, "SSE, FPU & IDT OK");

    if (hhdm == NULL)
        panic("error while getting HHDM", 0, 0);

    hhdm_offset = hhdm->offset;

    fbf_size = fb->pitch * fb->height;
    fbf_phys = (uintptr_t)fb->address - hhdm_offset;
    fbf_pitch = fb->pitch;
    fbf_width = fb->width;
    fbf_height = fb->height;

    gdt_tss_init();
    log(LOG_OK, "GDT & TSS OK");

    init_syscall(0x28, 0x30);
    log(LOG_OK, "Syscalls OK");

    if (rsdp == NULL)
        log(LOG_ERR, "Couldn't find RSDP address");
    else if (rsdp->address == NULL)
        log(LOG_ERR, "RSDP address is null");
    int acpi = acpi_init((RSDP_t *)rsdp->address);
    if (acpi)
        log(LOG_OK, "ACPI OK");
    else
        log(LOG_ERR, "Error while initializing ACPI");

    pmm_init(memmap, hhdm);
    log(LOG_OK, "PMM OK");
    extern vmm_table_t *vmm_init_kernel();
    kernel_pml4_virt = vmm_init_kernel();
    uintptr_t kernel_pml4_phys = (uintptr_t)kernel_pml4_virt - hhdm_offset;
    asm volatile("mov %0, %%cr3" ::"r"(kernel_pml4_phys) : "memory");
    log(LOG_OK, "VMM OK");

    uintptr_t heap_virtual_start = 0xFFFFFFFFC0000000;
    for (size_t offset = 0; offset < HEAP_SIZE; offset += PAGE_SIZE)
    {
        void *phys_frame = pmm_alloc_frame();
        if (!phys_frame)
            panic("PMM out of frames", 0, 0);
        vmm_map_page(kernel_pml4_virt, heap_virtual_start + offset, (uintptr_t)phys_frame, VMM_PRESENT | VMM_WRITE);
    }

    heap_init((void *)heap_virtual_start, HEAP_SIZE);
    stack_init(&system_stack);
    log(LOG_OK, "Heap & stack OK");

    extern uintptr_t lapic_base;
    uint32_t lapic_phys = (uint32_t)lapic_base;
    vmm_map_page(kernel_pml4_virt, lapic_phys + hhdm_offset, lapic_phys, VMM_PRESENT | VMM_WRITE | VMM_PCD | VMM_PWT);
    lapic_base = lapic_phys + hhdm_offset;
    extern volatile uintptr_t ioapic_base;

    extern volatile uintptr_t ioapic_base;
    extern uint32_t ioapic_phys;
    vmm_map_page(kernel_pml4_virt, (uintptr_t)ioapic_phys + hhdm_offset, ioapic_phys, VMM_PRESENT | VMM_WRITE | VMM_PCD | VMM_PWT);
    ioapic_base = (uintptr_t)ioapic_phys + hhdm_offset;

    lapic_init();
    lapic_timer_init(1000000);
    lapic_timer_calibrate();
    log(LOG_OK, "LAPIC OK");
    ioapic_set_irq(1, 0x21, 0);

    tmpfs_init();
    log(LOG_OK, "TMPFS OK");
    vfs_init();
    log(LOG_OK, "VFS OK");
    fat32_init();
    log(LOG_OK, "FAT32 OK");
    devfs_init();
    log(LOG_OK, "DEVFS OK");

    keyboard_init();
    register_framebuffer();

    task_init();

    log(LOG_INFO, "Loading initramfs");
    initramfs();

    asm volatile("sti");
    sched();

    while (1)
        ;
}
