/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <boot/limine.h>

struct GenericAddressStructure
{
    uint8_t AddressSpace;
    uint8_t BitWidth;
    uint8_t BitOffset;
    uint8_t AccessSize;
    uint64_t Address;
} __attribute__((packed)) typedef GAS_t;

struct ACPISDTHeader
{
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    uint8_t OEMID[6];
    uint8_t OEMTableID[8];
    char OEMRevision[4];
    char CreatorID[4];
    uint32_t CreatorRevision;
} __attribute__((packed)) typedef ACPISDTHeader;

struct FADT
{
    struct ACPISDTHeader h;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    uint8_t Reserved;

    uint8_t PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t AcpiEnable;
    uint8_t AcpiDisable;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t PM1EventLength;
    uint8_t PM1ControlLength;
    uint8_t PM2ControlLength;
    uint8_t PMTimerLength;
    uint8_t GPE0Length;
    uint8_t GPE1Length;
    uint8_t GPE1Base;
    uint8_t CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;

    uint16_t BootArchitectureFlags;

    uint8_t Reserved2;
    uint32_t Flags;

    GAS_t ResetReg;

    uint8_t ResetValue;
    uint8_t Reserved3[3];

    uint64_t X_FirmwareControl;
    uint64_t X_Dsdt;

    GAS_t X_PM1aEventBlock;
    GAS_t X_PM1bEventBlock;
    GAS_t X_PM1aControlBlock;
    GAS_t X_PM1bControlBlock;
    GAS_t X_PM2ControlBlock;
    GAS_t X_PMTimerBlock;
    GAS_t X_GPE0Block;
    GAS_t X_GPE1Block;
} __attribute__((packed)) typedef FADT_t;

struct RSDP
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) typedef RSDP_t;

int acpi_init(RSDP_t *rsdp);
void acpi_reboot();
void acpi_shutdown();

#endif
