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
    tbh I have no clue when will it be finally useful
    I guess never
*/

#include <bomboclaat/utsname.h>

struct utsname utsname = {
    .sysname = "Mierdux\0",
    .nodename = "bbcltOS\0",
    .release = "v1.0 beta 7.10.1\0",
    .version = "\0",
#ifdef __ARCH_X86_64
    .machine = "x86_64\0",
#elifdef __ARCH_RISCV64
    .machine = "riscv64\0",
#endif
};
