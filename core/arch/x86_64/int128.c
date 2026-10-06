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
// TO BE DELETED SOON
#include <x86_64/cpu.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/initramfs.h>
#include <tasks/loader.h>
#include <lib/string.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <memory/userspace.h>
#include <drivers/io.h>
#include <drivers/acpi.h>
#include <drivers/screen.h>
#include <tasks/tasks.h>
#include <tasks/exec.h>
#include <fs/vfs.h>
#include <stddef.h>
#include <errno.h>

#define TEMP_MAP_ADDR 0xFFFFFFFFF0000000

uint64_t int128_handler(context_t *r)
{
    switch (r->rax)
    {
    case 1: // printf
    {
        char *txt_ptr = (char *)r->rdi;
        uint32_t len = (uint32_t)r->rsi;
        if (len > 4096)
            len = 4096;

        char *txt = kmalloc(len + 1);
        if (!txt)
            return -ENOMEM;

        if (copy_from_user(txt, txt_ptr, len) != 0)
        {
            kfree(txt);
            return -EFAULT;
        }

        txt[len] = '\0';
        kprintf("%s", txt);
        kfree(txt);
        return 0;
    }
    case 4:
    {
        // TODO: get rid of this and write to /dev/kbd
        char *buf = (char *)r->rdi;
        uint32_t max_len = (uint32_t)r->rsi;
        if (max_len == 0 || max_len > 4096)
            return -EINVAL;

        char *tmpbuf = kmalloc(max_len);
        if (!tmpbuf)
            return -ENOMEM;

        input(tmpbuf, max_len);
        tmpbuf[max_len - 1] = '\0';
        int res = copy_to_user(buf, tmpbuf, strlen(tmpbuf) + 1);
        kfree(tmpbuf);

        return res == 0 ? 0 : -EFAULT;
    }
    default:
        return -ENOSYS;
    }
}
