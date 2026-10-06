/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EXEC_H
#define EXEC_H

#include <tasks/tasks.h>

/*
    I saw this one in FreeBSD's code and I thought it looks nice
    so I stole the idea and here it is :)
    All pointers in struct execve_args are USER pointers.
*/

#ifdef __cplusplus
extern "C"
{
#endif
    struct execve_args
    {
        char *path;
        char **argv;
        char **envp;
    };

    struct image
    {
        vmm_table_t *pml4;
        mm_t mm;
        uint64_t entry;
        uint64_t rsp;
        uint64_t argv;
        uint64_t argc;
        uint64_t sigterm;
    };

    int execve(struct execve_args args);
    int spawn(struct execve_args args);
    int image_build(const void *elf, size_t size, int argc, char **argv, int stack_pages, struct image *out);
    void image_free(struct image *img);
    void enter_user_context(context_t *ctx, uintptr_t kstack_top) __attribute__((noreturn));
#ifdef __cplusplus
}
#endif

#endif
