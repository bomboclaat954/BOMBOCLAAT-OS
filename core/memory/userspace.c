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
#include <memory/pmm.h>
#include <memory/memtools.h>
#include <tasks/tasks.h>
#include <bomboclaat/kprintf.h>

extern uint64_t hhdm_offset;

static int user_range_valid(uintptr_t addr, uint64_t len)
{
    if (len == 0)
        return 1;
    if (addr < USER_MIN_ADDR || addr >= USER_SPACE_LIMIT)
        return 0;
    if (len > USER_SPACE_LIMIT - addr)
        return 0;
    return 1;
}

static int user_page_translate(uintptr_t addr, uint64_t need_flags, uint8_t **kptr)
{
    uintptr_t phys = 0;
    uint64_t flags = 0;

    if (vmm_resolve(current_task->pml4, addr, &phys, &flags) != 0)
        return -1;
    if ((flags & need_flags) != need_flags)
        return -1;

    *kptr = (uint8_t *)(phys + hhdm_offset);
    return 0;
}

int copy_to_user(void *dst, void *src, uint32_t len)
{
    uintptr_t va = (uintptr_t)dst;
    uint8_t *from = (uint8_t *)src;

    if (!current_task || !user_range_valid(va, len))
        return -1;

    while (len)
    {
        uint8_t *kptr;
        if (user_page_translate(va, VMM_USER | VMM_WRITE, &kptr) != 0)
            return -1;

        uint32_t chunk = PAGE_SIZE - (va & (PAGE_SIZE - 1));
        if (chunk > len)
            chunk = len;

        memcpy(kptr, from, chunk);
        va += chunk;
        from += chunk;
        len -= chunk;
    }

    return 0;
}

int copy_from_user(void *dst, void *src, uint32_t len)
{
    uintptr_t va = (uintptr_t)src;
    uint8_t *to = (uint8_t *)dst;

    if (!current_task || !user_range_valid(va, len))
        return -1;

    while (len)
    {
        uint8_t *kptr;
        if (user_page_translate(va, VMM_USER, &kptr) != 0)
            return -1;

        uint32_t chunk = PAGE_SIZE - (va & (PAGE_SIZE - 1));
        if (chunk > len)
            chunk = len;

        memcpy(to, kptr, chunk);
        va += chunk;
        to += chunk;
        len -= chunk;
    }

    return 0;
}

int copy_string_from_user(char *dst, const char *src, uint32_t max_len)
{
    uintptr_t va = (uintptr_t)src;

    if (!current_task || max_len == 0 || !user_range_valid(va, 1))
        return -1;

    uint32_t copied = 0;
    while (copied < max_len)
    {
        uint8_t *kptr;
        if (va >= USER_SPACE_LIMIT || user_page_translate(va, VMM_USER, &kptr) != 0)
            return -1;

        uint32_t chunk = PAGE_SIZE - (va & (PAGE_SIZE - 1));
        for (uint32_t i = 0; i < chunk && copied < max_len; i++)
        {
            char c = (char)kptr[i];
            dst[copied++] = c;
            if (c == '\0')
                return (int)(copied - 1);
        }
        va += chunk;
    }

    dst[max_len - 1] = '\0';
    return -1;
}
