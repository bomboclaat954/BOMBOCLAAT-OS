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
#define KERNEL_STACK_FRAMES 4

#define USER_SPACE_LIMIT 0x0000800000000000ULL
#define USER_MIN_ADDR 0x1000ULL
#define USER_STACK_TOP 0x00007FFFFFFFF000ULL
#define USER_STACK_PAGES 4
#define USER_STACK_MAX_PAGES 256
#define USER_FB_VIRT 0x00007FFF00000000ULL
#define USER_SIGTRAMP_VIRT 0x00007FFE00000000ULL
#define USER_IMAGE_LIMIT USER_SIGTRAMP_VIRT
#define USER_HEAP_MAX 0x10000000ULL
#define USER_RFLAGS_MASK 0xCD5ULL
#define USER_CS 0x43
#define USER_SS 0x3B
#define KERNEL_CS 0x28
#define KERNEL_SS 0x30

typedef registers_t context_t;

typedef enum
{
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE,
    TASK_NEW,
} task_state_t;

/*
    Task's family explained:
        * parent: the process that hit fork() to create a child
        * children: list of processes that were created by a process by fork()
        * siblings: processes created by the same parent
*/
struct task
{
    pid_t pid;
    struct task *parent;
    list_head_t children;
    list_head_t sibling;
    task_state_t state;
    vmm_table_t *pml4;
    uintptr_t kstack_top;
    uintptr_t kernel_rsp;
    uintptr_t kstack_frames[KERNEL_STACK_FRAMES];
    context_t cpu_ctx;
    mm_t mm;
    char name[64];
    struct task *next;
    vfs_file_t *fd_table[MAX_FILES_PER_TASK];
    vfs_inode_t *current_dir;
    sig_t pending_signal;
    uint64_t sigterm_handler_rip;
    int exit_code;
    uint64_t cpu_time;
    uint8_t fpu_raw[528];
} typedef task_t;

void task_init(void);
task_t *find_by_pid(int pid);
int find_in_array(task_t *t);
int task_insert(task_t *t);
task_t *find_just_forked();
task_t *task_alloc(void);
task_t *task_create(void *elf_data, size_t elf_size, int parent_pid, char *name, int argc, char **argv, int frames);
pid_t new_pid();
void sched(void);
void task_set_name(task_t *t, const char *name);
int task_alloc_kstack(task_t *t);
uintptr_t build_kernel_frame(task_t *task);
uintptr_t build_kernel_frame_from_syscall(task_t *task);
void task_terminate(task_t *t, int code);
void task_reap(task_t *t);
void task_reap_orphans(void);
void task_release(task_t *t);
struct image;
task_t *task_from_image(task_t *parent, struct image *img, const char *name);
void task_setup_user_context(task_t *t, const struct image *img);
int task_setup_user_stack(vmm_table_t *pml4, mm_t *mm, int argc, char **argv, int pages, uintptr_t *rsp_out, uintptr_t *argv_out);
int task_map_shared_regions(vmm_table_t *pml4);
uint64_t task_sbrk(task_t *t, int64_t increment);
void *task_fpu_area(task_t *t);
void task_fpu_init(task_t *t);
void task_fpu_save(task_t *t);
void task_fpu_load(task_t *t);
int vmm_is_shared_frame(uintptr_t phys);
void cpu_switch_context(uintptr_t *prev_rsp, uintptr_t next_rsp);
int find_free_slot(void);
void task_link(task_t *parent, task_t *child);
void task_inherit_fds(task_t *child, task_t *parent);
void kernel_idle_loop(void) __attribute__((noreturn));
void signal_check_user(uint64_t *user_rip, uint64_t *user_rsp);

static inline uint64_t irq_save(void)
{
    uint64_t flags;
    asm volatile("pushfq\n\tpopq %0\n\tcli" : "=r"(flags)::"memory");
    return flags;
}

static inline void irq_restore(uint64_t flags)
{
    if (flags & 0x200)
        asm volatile("sti" ::: "memory");
}

extern task_t *current_task;
extern task_t *kernel_task;
extern task_t *tasks[MAX_TASKS];
extern uint64_t syscall_kernel_rsp;

#endif
