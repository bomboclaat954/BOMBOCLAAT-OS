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

#include <memory/userspace.h>
#include <memory/vmm.h>
#include <memory/memtools.h>
#include <tasks/tasks.h>
#include <bomboclaat/kprintf.h>

int copy_to_user(void *dst, void *src, uint32_t len)
{
    extern task_t *current_task;
    extern uint64_t hhdm_offset;

    uintptr_t phys = 0;
    uint64_t flags = 0;

    if (vmm_resolve(current_task->pml4, (uintptr_t)dst, &phys, &flags) != 0)
        return -1;

    uint8_t *dst_ptr = (uint8_t *)(phys + hhdm_offset);
    memcpy(dst_ptr, src, len);

    return 0;
}
