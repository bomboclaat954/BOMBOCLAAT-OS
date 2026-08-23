/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stddef.h>
#include <fs/vfs.h>
#include <memory/vmm.h>
#include <bomboclaat/types.h>

#define MAX_FILES_PER_TASK 32
#define MAX_TASKS 32
#define TASK_STACK_SENTINEL 0xC0FFEE00C0FFEE00ULL

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
    TASK_ZOMBIE,
    TASK_NEW,
} task_state_t;

/*
    Process' family explained:
        * parent: the process that hit fork() to create a child
        * children: list of processes that were created by a process by fork()
        * siblings: processes created by the same parent
*/
struct task
{
    pid_t pid;

    struct task *parent;
    list_head_t children;
    list_head_t sibling; // why in singular form?
    task_state_t state;

    vmm_table_t *pml4;
    uintptr_t kstack_top;
    uintptr_t kstack_frames[4];
    uintptr_t rsp;
    mm_t *mm; // not used in old task_create, but used in execve

    char name[64];
    struct task *next;

    vfs_file_t *fd_table[MAX_FILES_PER_TASK];
} typedef task_t;

void task_init(void);
int task_insert(task_t *t);
task_t *find_just_forked();
task_t *task_create(void *elf_data, int parent_pid, char *name, int argc, char **argv, int frames);
context_t *schedule(context_t *ctx);
void task_exit(context_t *ctx);
pid_t new_pid();

extern void switch_to_task(uintptr_t next_rsp, uintptr_t next_cr3) __attribute__((noreturn));

#endif
