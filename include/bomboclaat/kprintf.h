/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KPRINTF_H
#define KPRINTF_H

#ifdef __cplusplus
extern "C"
{
#endif
    enum log_type
    {
        LOG_OK,
        LOG_ERR,
        LOG_INFO,
        LOG_DEBUG,
    } typedef log_type;

    int kprintf(const char *fmt, ...);
    int sprintf(char *buf, const char *fmt, ...);
    int log(log_type type, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
