/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#define _UTSNAME_LENGTH 65

struct utsname
{
    char sysname[_UTSNAME_LENGTH];  // Name of this implementation of the operating system.
    char nodename[_UTSNAME_LENGTH]; // Name of this node within the communications network to which this node is attached, if any.
    char release[_UTSNAME_LENGTH];  // Current release level of this implementation.
    char version[_UTSNAME_LENGTH];  // Current version level of this release.
    char machine[_UTSNAME_LENGTH];  // Name of the hardware type on which the system is running.
};

int uname(struct utsname *_utsname);
