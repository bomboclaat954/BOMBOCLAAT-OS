/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026  Jakub Fietko <fietkojakub@proton.me>
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

#include <drivers/acpi.h>
#include <drivers/io.h>
#include <memory/memtools.h>
#include <memory/vmm.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/panic.h>

// in case if you're running this on a dinosaur computer
void legacy_reboot()
{
    unsigned char good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
}

void legacy_shutdown()
{
    outw(0xB004, 0x2000); // QEMU
    outw(0x604, 0x2000);  // QEMU / Bochs
    outw(0x4004, 0x3400); // VirtualBox
}

// to the person who invented ACPI: fuck you
FADT_t *fadt = NULL;
MADT_header_t *madt = NULL;
uint32_t ioapic_phys = 0;
volatile uintptr_t ioapic_base = 0;

void *acpi_find_table(ACPISDTHeader *main_table, const char *signature)
{
    if (main_table == NULL)
        return NULL;

    int is_xsdt = (memcmp(main_table->Signature, "XSDT", 4) == 0);
    int is_rsdt = (memcmp(main_table->Signature, "RSDT", 4) == 0);

    if (!is_xsdt && !is_rsdt)
        return NULL;

    int pointer_size = is_xsdt ? 8 : 4;
    int entries = (main_table->Length - sizeof(ACPISDTHeader)) / pointer_size;
    uintptr_t ptr_start = (uintptr_t)main_table + sizeof(ACPISDTHeader);

    for (int i = 0; i < entries; i++)
    {
        uintptr_t phys_addr = 0;

        if (is_xsdt)
            phys_addr = ((uint64_t *)ptr_start)[i];
        else
            phys_addr = ((uint32_t *)ptr_start)[i];

        if (phys_addr == 0)
            continue;

        ACPISDTHeader *table = (ACPISDTHeader *)(phys_addr + hhdm_offset);

        if (memcmp(table->Signature, signature, 4) == 0)
            return (void *)table;
    }

    return NULL;
}

int acpi_init(RSDP_t *rsdp)
{
    if (!(memcmp((void *)rsdp->signature, (void *)"RSD PTR ", 8) == 0))
        return 0;

    ACPISDTHeader *main_table = NULL;

    if (rsdp->revision >= 2 && rsdp->xsdt_address != 0)
        main_table = (ACPISDTHeader *)(uintptr_t)(rsdp->xsdt_address + hhdm_offset);
    else
        main_table = (ACPISDTHeader *)(uintptr_t)(rsdp->rsdt_address + hhdm_offset);

    fadt = (FADT_t *)acpi_find_table(main_table, "FACP");
    madt = (MADT_header_t *)((uintptr_t)acpi_find_table(main_table, "APIC"));

    extern volatile uintptr_t lapic_base;
    lapic_base = (uintptr_t)madt->LAPIC_addr;
    log(LOG_INFO, "LAPIC addr: %x", madt->LAPIC_addr);

    MADT_record_header_t *rhdr = (MADT_record_header_t *)((uintptr_t)madt + sizeof(MADT_header_t));
    uintptr_t madt_end = (uintptr_t)madt + madt->acpihdr.Length;

    if ((uintptr_t)rhdr > madt_end)
        log(LOG_ERR, "MADT has no records or size mismatch");

    while ((uintptr_t)rhdr < madt_end)
    {
        if (rhdr->entry_type == 1)
        {
            IOAPIC_t *ioapic = (IOAPIC_t *)rhdr;
            log(LOG_INFO, "IOAPIC addr: %x", ioapic->addr);
            ioapic_phys = ioapic->addr;
            break;
        }

        if (rhdr->record_len == 0)
            break;
        rhdr = (MADT_record_header_t *)((uintptr_t)rhdr + rhdr->record_len);
    }

    if (fadt && madt)
        return 1;
    else
        return 0;
}

void acpi_reboot()
{
    GAS_t *reg = &fadt->ResetReg;
    uint8_t val = fadt->ResetValue;

    switch (reg->AddressSpace)
    {
    case 0x00:
        *((volatile uint8_t *)(uintptr_t)(reg->Address + hhdm_offset)) = val;
        break;
    case 0x01:
        outb((uint16_t)reg->Address, val);
        break;
    case 0x02:
        break;
    }

    log(LOG_ERR, "ACPI reboot failed, trying legacy");
    legacy_reboot();
    panic("couldn't reboot", 0, 0);
}

uint32_t aml_parse_pkglength(uint8_t *data, uint32_t *out_bytes_used)
{
    uint8_t lead = data[0];
    uint8_t extra = (lead >> 6) & 0x3;

    if (extra == 0)
    {
        *out_bytes_used = 1;
        return lead & 0x3F;
    }

    uint32_t len = lead & 0x0F;
    for (uint8_t i = 1; i <= extra; i++)
        len |= ((uint32_t)data[i]) << (4 + (i - 1) * 8);

    *out_bytes_used = 1 + extra;
    return len;
}

void write_pm1_control(uint16_t val)
{
    if (fadt->X_PM1aControlBlock.Address != 0)
    {
        switch (fadt->X_PM1aControlBlock.AddressSpace)
        {
        case 0x01:
            outw((uint16_t)fadt->X_PM1aControlBlock.Address, val);
            break;
        case 0x00:
            *((volatile uint16_t *)(uintptr_t)(fadt->X_PM1aControlBlock.Address + hhdm_offset)) = val;
            break;
        }
    }
    else if (fadt->PM1aControlBlock != 0)
        outw((uint16_t)fadt->PM1aControlBlock, val);

    if (fadt->X_PM1bControlBlock.Address != 0)
    {
        switch (fadt->X_PM1bControlBlock.AddressSpace)
        {
        case 0x01:
            outw((uint16_t)fadt->X_PM1bControlBlock.Address, val);
            break;
        case 0x00:
            *((volatile uint16_t *)(uintptr_t)(fadt->X_PM1bControlBlock.Address + hhdm_offset)) = val;
            break;
        }
    }
    else if (fadt->PM1bControlBlock != 0)
        outw((uint16_t)fadt->PM1bControlBlock, val);
}

int acpi_parse_s5(uint8_t *slp_typa, uint8_t *slp_typb)
{
    uintptr_t dsdt_phys = (fadt->X_Dsdt != 0) ? (uintptr_t)fadt->X_Dsdt : (uintptr_t)fadt->Dsdt;

    if (dsdt_phys == 0)
        return 0;

    uint8_t *dsdt = (uint8_t *)(dsdt_phys + hhdm_offset);
    uint32_t len = ((ACPISDTHeader *)dsdt)->Length;

    for (uint32_t i = 1; i < len - 4; i++)
    {
        if (!(dsdt[i] == '_' && dsdt[i + 1] == 'S' &&
              dsdt[i + 2] == '5' && dsdt[i + 3] == '_'))
            continue;

        if (dsdt[i - 1] != 0x08 && dsdt[i - 1] != 0x10)
            continue;

        uint8_t *pkg = &dsdt[i + 4];

        if (pkg[0] != 0x12)
            continue;
        pkg++;

        uint32_t pkglen_bytes = 0;
        aml_parse_pkglength(pkg, &pkglen_bytes);
        pkg += pkglen_bytes;

        pkg++;

        if (pkg[0] == 0x0A)
            pkg++;
        *slp_typa = pkg[0] & 0x1F;
        pkg++;

        if (pkg[0] == 0x0A)
            pkg++;
        *slp_typb = pkg[0] & 0x1F;

        return 1;
    }
    return 0;
}

void acpi_shutdown(void)
{
    uint8_t slp_typa = 0, slp_typb = 0;

    if (!acpi_parse_s5(&slp_typa, &slp_typb))
    {
        log(LOG_ERR, "ACPI: _S5_ not found in DSDT");
        goto fallback;
    }

    if (fadt->SMI_CommandPort != 0 && fadt->AcpiEnable != 0)
        outb((uint16_t)fadt->SMI_CommandPort, fadt->AcpiEnable);

    uint16_t val = (slp_typa << 10) | (1 << 13);
    write_pm1_control(val);

fallback:
    log(LOG_ERR, "ACPI shutdown failed, trying legacy");
    legacy_shutdown();
    panic("couldn't shut down", 0, 0);
}
