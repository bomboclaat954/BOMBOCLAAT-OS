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

// to the person who invented ACPI: fuck you, asshole

void *acpi_find_table(ACPISDTHeader *main_table, const char *signature)
{
    if (main_table == NULL)
        return NULL;

    int is_xsdt = (memcmp((void *)main_table->Signature, "XSDT", 4) == 0);
    int is_rsdt = (memcmp((void *)main_table->Signature, "RSDT", 4) == 0);

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

        ACPISDTHeader *table = (ACPISDTHeader *)(phys_addr + hhdm_offset);

        if (phys_addr == 0)
            continue;

        if (memcmp((void *)table->Signature, (void *)signature, 4) == 0)
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

    FADT_t *fadt = (FADT_t *)acpi_find_table(main_table, "FACP");
    if (fadt != NULL)
        return 1;
    else
        return 0;
}

void acpi_reboot()
{
}

void acpi_shutdown()
{
}
