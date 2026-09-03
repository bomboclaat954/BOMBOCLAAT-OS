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
#include <x86_64/cpu.h>

#define MAX_FILES_PER_TASK 32
#define MAX_TASKS 32
#define TASK_STACK_SENTINEL 0xC0FFEE00C0FFEE00ULL

typedef registers_t context_t; // same struct btw

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
    uintptr_t kernel_rsp;
    uintptr_t kstack_frames[4];
    context_t cpu_ctx;
    mm_t mm;
    char name[64];
    struct task *next;
    vfs_file_t *fd_table[MAX_FILES_PER_TASK];
    vfs_inode_t *current_dir;
    int exit_code;
} typedef task_t;

void task_init(void);
task_t *find_by_pid(int pid);
int find_in_array(task_t *t);
int task_insert(task_t *t);
task_t *find_just_forked();
task_t *task_create(void *elf_data, int parent_pid, char *name, int argc, char **argv, int frames);
pid_t new_pid();
void sched(void);

extern void cpu_switch_context(uintptr_t *old_rsp, uintptr_t new_rsp);
extern task_t *current_task;

#endif
