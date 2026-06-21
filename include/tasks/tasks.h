/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stddef.h>

typedef struct vmm_table vmm_table_t;

typedef struct
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no;
    uint64_t err_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) context_t;

typedef enum
{
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

typedef struct task
{
    int pid;
    int parent_pid;
    task_state_t state;
    vmm_table_t *pml4;
    uintptr_t kstack_top;
    uintptr_t rsp;
    char name[16];
    struct task *next;
} task_t;

extern volatile int need_reschedule;

void task_init(void);
task_t *task_create(void *elf_data, int parent_pid, char *name, int frames);
context_t *schedule(context_t *ctx);
void task_exit(context_t *ctx);

extern void switch_to_task(uintptr_t next_rsp, uintptr_t next_cr3) __attribute__((noreturn));

#endif
