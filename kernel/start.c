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

/*
    Dear user,
    I really want to thank you for downloading and viweing this code. I'm really happy that you're interested in this.
    If you're a professional OS developer and you see some bugs or errors here, please contact me, I'd really appreciate that.
    Enjoy the code and write a feedback if you want. May God bless you,
    Jakub Fietko, the author.
*/

#include <bomboclaat/kprintf.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/initramfs.h>
#include <drivers/io.h>
#include <drivers/screen.h>
#include <drivers/ata.h>
#include <drivers/serial.h>
#include <drivers/acpi.h>
#include <boot/limine.h>
#include <int/int.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/kmalloc.h>
#include <fs/fat32.h>
#include <fs/ramfs.h>
#include <tasks/tasks.h>
#include <lib/string.h>
#include <lib/math.h>

char *UNAME[4];
char *kname = "BOMBOCLAAT Kernel";
char *krelease = "1.0-beta2";

stack_t system_stack;
global_settings settings;

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
    idt_init();
    pit_init();
    log(LOG_OK, "Enabled SSE, FPU, IDT and PIT");
    gdt_tss_init();
    log(LOG_OK, "Initialized GDT and TSS");

    if (memmap == NULL || memmap->entry_count == 0)
        panic("unable to get memory map", 0, 0);
    if (hhdm == NULL)
        panic("error while getting HHDM", 0, 0);

    hhdm_offset = hhdm->offset;
    log(LOG_INFO, "HHDM offset: %x", hhdm_offset);

    log(LOG_INFO, "Getting memory map");
    ram_t mem = init_memmap(memmap);
    if (mem.total == 0 || mem.usable == 0)
        panic("error while getting RAM size", 0, 0);
    log(LOG_OK, "Initialized memory map");
    pmm_init(memmap, hhdm);
    log(LOG_OK, "Initialized PMM");
    extern vmm_table_t *vmm_init_kernel();
    kernel_pml4_virt = vmm_init_kernel();
    uintptr_t kernel_pml4_phys = (uintptr_t)kernel_pml4_virt - hhdm_offset;
    asm volatile("mov %0, %%cr3" ::"r"(kernel_pml4_phys) : "memory");
    log(LOG_OK, "Initialized VMM");

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
    log(LOG_OK, "Set up heap and system stack");

    char buf[64];
    sprintf(buf, "Total detected RAM: %d MB", mem.total / (1024 * 1024));
    log(LOG_INFO, buf);

    ramfs_t *ramfs = init_ramfs();
    if (ramfs)
        log(LOG_OK, "Initialized RAMFS");
    else
        log(LOG_ERR, "Error while initializing RAMFS");
    task_init();
    UNAME[0] = kmalloc(sizeof(char) * 128);
    UNAME[1] = kmalloc(sizeof(char) * 128);
    UNAME[2] = kmalloc(sizeof(char) * 128);
    UNAME[3] = kmalloc(sizeof(char) * 128);
    strcpy(kname, UNAME[0]);
    strcpy(krelease, UNAME[1]);
    sprintf(UNAME[2], "%d", BUILD_NUMBER);
    get_cpu_model(UNAME[3]);

    settings.fat32 = 0;
    // settings.fat32 = init_fat32();
    if (settings.fat32)
    {
        log(LOG_INFO, "Found an ATA disk with FAT32");
        settings.current_dir_cluster = settings.fat32;
    }

    int serial = init_serial();
    if (serial == 0)
        log(LOG_OK, "Serial port initialized");
    else
        log(LOG_ERR, "Error while initializing serial port");

    if (rsdp == NULL)
        log(LOG_ERR, "Couldn't find RSDP address");
    else if (rsdp->address == NULL)
        log(LOG_ERR, "RSDP address is null");
    log(LOG_OK, "Found RSDP at 0x%x", rsdp->address);
    int acpi = acpi_init((RSDP_t *)rsdp->address);
    if (acpi)
        log(LOG_OK, "Initialized ACPI");
    else
        log(LOG_ERR, "Error while initializing ACPI");

    log(LOG_INFO, "Loading initramfs");
    initramfs();
    asm volatile("sti");
    extern void reg_dump(registers_t * r);
    context_t *ctx = (context_t *)kmalloc(sizeof(context_t));
    reg_dump((registers_t *)ctx); // registers_t and context_t are the same
    schedule(ctx);
    while (1)
        asm volatile("hlt");
}
