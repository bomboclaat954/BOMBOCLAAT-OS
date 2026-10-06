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

#include <tasks/tasks.h>
#include <tasks/exec.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <x86_64/gdt_tss.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/globals.h>
#include <lib/string.h>
#include <errno.h>

extern vmm_table_t *kernel_pml4_virt;
extern uint64_t hhdm_offset;
extern uintptr_t fbf_phys;
extern uintptr_t fbf_size;
extern void ret_from_fork(void);

task_t *tasks[MAX_TASKS] = {NULL};
task_t *current_task = NULL;
task_t *kernel_task = NULL;
static int next_pid = 1;

static uint8_t fpu_default_image[512] __attribute__((aligned(16)));
static uintptr_t sigtramp_phys = 0;

static const uint8_t sigtramp_code[] = {
    0xBF, 0x82, 0x00, 0x00, 0x00,
    0xB8, 0x05, 0x00, 0x00, 0x00,
    0x0F, 0x05,
    0xEB, 0xFE,
};

void *task_fpu_area(task_t *t)
{
    return (void *)(((uintptr_t)t->fpu_raw + 15) & ~15ULL);
}

void task_fpu_init(task_t *t)
{
    memcpy((uint8_t *)task_fpu_area(t), fpu_default_image, 512);
}

void task_fpu_save(task_t *t)
{
    asm volatile("fxsave64 (%0)" ::"r"(task_fpu_area(t)) : "memory");
}

void task_fpu_load(task_t *t)
{
    asm volatile("fxrstor64 (%0)" ::"r"(task_fpu_area(t)) : "memory");
}

int vmm_is_shared_frame(uintptr_t phys)
{
    phys &= ~(uintptr_t)(PAGE_SIZE - 1);
    if (sigtramp_phys && phys == sigtramp_phys)
        return 1;
    if (fbf_size && phys >= (fbf_phys & ~(uintptr_t)(PAGE_SIZE - 1)) && phys < fbf_phys + fbf_size)
        return 1;
    return 0;
}

void task_set_name(task_t *t, const char *name)
{
    size_t i = 0;
    if (name)
    {
        for (; i < sizeof(t->name) - 1 && name[i]; i++)
            t->name[i] = name[i];
    }
    t->name[i] = '\0';
}

int task_alloc_kstack(task_t *t)
{
    uintptr_t base = (uintptr_t)pmm_alloc_contiguous(KERNEL_STACK_FRAMES);
    if (!base)
        return -ENOMEM;

    for (int i = 0; i < KERNEL_STACK_FRAMES; i++)
        t->kstack_frames[i] = base + (uintptr_t)i * PAGE_SIZE;

    t->kstack_top = base + hhdm_offset + (uintptr_t)KERNEL_STACK_FRAMES * PAGE_SIZE;
    *(uint64_t *)(base + hhdm_offset) = TASK_STACK_SENTINEL;
    return 0;
}

static void task_free_kstack(task_t *t)
{
    if (!t->kstack_frames[0])
        return;
    for (int i = 0; i < KERNEL_STACK_FRAMES; i++)
        pmm_free_frame((void *)t->kstack_frames[i]);
    t->kstack_frames[0] = 0;
    t->kstack_top = 0;
}

uintptr_t build_kernel_frame(task_t *task)
{
    context_t *frame = (context_t *)(task->kstack_top - sizeof(context_t));
    uint64_t *callee_saved = (uint64_t *)frame - 7;

    for (int i = 0; i < 6; i++)
        callee_saved[i] = 0;
    callee_saved[6] = (uint64_t)ret_from_fork;

    *frame = task->cpu_ctx;
    frame->int_no = 0;
    frame->error_code = 0;

    task->kernel_rsp = (uintptr_t)callee_saved;
    return task->kernel_rsp;
}

uintptr_t build_kernel_frame_from_syscall(task_t *task)
{
    return build_kernel_frame(task);
}

int find_free_slot(void)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (!tasks[i])
            return i;
    }
    return -1;
}

int find_in_array(task_t *t)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i] == t)
            return i;
    }
    return -1;
}

task_t *find_by_pid(int pid)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i] != NULL && tasks[i]->pid == pid)
            return tasks[i];
    }
    return NULL;
}

int task_insert(task_t *t)
{
    int slot = find_free_slot();
    if (slot < 0)
        return -1;

    tasks[slot] = t;
    return 0;
}

task_t *find_just_forked()
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i] && tasks[i]->state == TASK_NEW)
            return tasks[i];
    }
    return NULL;
}

pid_t new_pid()
{
    return next_pid++;
}

void kernel_idle_loop(void)
{
    log(LOG_INFO, "Entering kernel idle loop");
    while (1)
    {
        task_reap_orphans();
        asm volatile("sti\n\thlt" ::: "memory");
    }
}

task_t *task_alloc(void)
{
    task_t *t = (task_t *)kmalloc(sizeof(task_t));
    if (!t)
        return NULL;
    memset(t, 0, sizeof(task_t));

    if (task_alloc_kstack(t) != 0)
    {
        kfree(t);
        return NULL;
    }

    INIT_LIST_HEAD(&t->children);
    INIT_LIST_HEAD(&t->sibling);
    task_fpu_init(t);
    t->state = TASK_NEW;
    return t;
}

void task_init(void)
{
    memset(fpu_default_image, 0, sizeof(fpu_default_image));
    *(uint16_t *)(fpu_default_image + 0) = 0x037F;
    *(uint32_t *)(fpu_default_image + 24) = 0x1F80;

    void *tramp = pmm_alloc_frame_zeroed();
    if (!tramp)
        panic("task_init: no memory for signal trampoline", 0, 0);
    sigtramp_phys = (uintptr_t)tramp;
    memcpy((uint8_t *)(sigtramp_phys + hhdm_offset), (uint8_t *)sigtramp_code, sizeof(sigtramp_code));

    task_t *k = (task_t *)kmalloc(sizeof(task_t));
    if (!k)
        panic("task_init: out of memory", 0, 0);
    memset(k, 0, sizeof(task_t));

    if (task_alloc_kstack(k) != 0)
        panic("task_init: no memory for kernel stack", 0, 0);

    k->pid = 0;
    task_set_name(k, "kernel");
    k->state = TASK_RUNNING;
    k->pml4 = kernel_pml4_virt;
    k->cpu_ctx.cs = KERNEL_CS;
    k->cpu_ctx.ss = KERNEL_SS;
    k->cpu_ctx.rflags = 0x202;
    k->next = k;
    k->parent = k;
    INIT_LIST_HEAD(&k->children);
    INIT_LIST_HEAD(&k->sibling);
    task_fpu_init(k);

    kernel_task = k;
    current_task = k;
    tasks[0] = k;
}

int task_map_shared_regions(vmm_table_t *pml4)
{
    uintptr_t aligned_size = ((uintptr_t)fbf_size + PAGE_SIZE - 1) & ~(uintptr_t)(PAGE_SIZE - 1);
    uintptr_t fb_base = fbf_phys & ~(uintptr_t)(PAGE_SIZE - 1);

    for (uintptr_t offset = 0; offset < aligned_size; offset += PAGE_SIZE)
    {
        if (vmm_map_page(pml4, USER_FB_VIRT + offset, fb_base + offset, VMM_PRESENT | VMM_WRITE | VMM_USER) != 0)
            return -ENOMEM;
    }

    if (vmm_map_page(pml4, USER_SIGTRAMP_VIRT, sigtramp_phys, VMM_PRESENT | VMM_USER) != 0)
        return -ENOMEM;

    return 0;
}

int task_setup_user_stack(vmm_table_t *pml4, mm_t *mm, int argc, char **argv, int pages, uintptr_t *rsp_out, uintptr_t *argv_out)
{
    if (pages <= 0)
        pages = USER_STACK_PAGES;
    if (pages > USER_STACK_MAX_PAGES)
        pages = USER_STACK_MAX_PAGES;
    if (argc < 0)
        return -EINVAL;

    uintptr_t stack_bottom = USER_STACK_TOP - (uintptr_t)pages * PAGE_SIZE;
    uintptr_t top_phys = 0;

    for (int i = 0; i < pages; i++)
    {
        void *frame = pmm_alloc_frame_zeroed();
        if (!frame)
            return -ENOMEM;

        if (vmm_map_page(pml4, stack_bottom + (uintptr_t)i * PAGE_SIZE, (uintptr_t)frame, VMM_PRESENT | VMM_WRITE | VMM_USER) != 0)
        {
            pmm_free_frame(frame);
            return -ENOMEM;
        }

        if (i == pages - 1)
            top_phys = (uintptr_t)frame;
    }

    uintptr_t top_kvirt = top_phys + hhdm_offset;
    size_t offset = PAGE_SIZE;

    uintptr_t *user_addrs = (uintptr_t *)kmalloc(sizeof(uintptr_t) * ((size_t)argc + 1));
    if (!user_addrs)
        return -ENOMEM;

    for (int i = argc - 1; i >= 0; i--)
    {
        size_t len = argv[i] ? ((size_t)strlen(argv[i]) + 1) : 1;
        if (len > offset)
        {
            kfree(user_addrs);
            return -E2BIG;
        }

        offset -= len;
        char *dest = (char *)(top_kvirt + offset);
        if (argv[i])
            memcpy((uint8_t *)dest, (uint8_t *)argv[i], (uint32_t)len);
        else
            dest[0] = '\0';

        user_addrs[i] = USER_STACK_TOP - (PAGE_SIZE - offset);
    }
    user_addrs[argc] = 0;

    size_t need = ((size_t)argc + 2) * sizeof(uintptr_t);
    if (offset < need)
    {
        kfree(user_addrs);
        return -E2BIG;
    }

    size_t final_off = (offset - need) & ~(size_t)0xF;

    *(uint64_t *)(top_kvirt + final_off) = (uint64_t)argc;
    memcpy((uint8_t *)(top_kvirt + final_off + 8), (uint8_t *)user_addrs, (uint32_t)((size_t)(argc + 1) * sizeof(uintptr_t)));

    *rsp_out = USER_STACK_TOP - (PAGE_SIZE - final_off);
    *argv_out = *rsp_out + 8;

    mm->stack_start = USER_STACK_TOP;
    mm->stack_limit = USER_STACK_TOP - (uintptr_t)USER_STACK_MAX_PAGES * PAGE_SIZE;

    kfree(user_addrs);
    return 0;
}

void task_setup_user_context(task_t *t, const struct image *img)
{
    memset(&t->cpu_ctx, 0, sizeof(context_t));
    t->cpu_ctx.rip = img->entry;
    t->cpu_ctx.rsp = img->rsp;
    t->cpu_ctx.cs = USER_CS;
    t->cpu_ctx.ss = USER_SS;
    t->cpu_ctx.rflags = 0x202;
    t->cpu_ctx.rdi = img->argc;
    t->cpu_ctx.rsi = img->argv;
}

void task_inherit_fds(task_t *child, task_t *parent)
{
    for (int i = 0; i < MAX_FILES_PER_TASK; i++)
    {
        if (parent->fd_table[i])
        {
            child->fd_table[i] = parent->fd_table[i];
            child->fd_table[i]->ref_count++;
        }
    }
    child->current_dir = parent->current_dir;
}

void task_link(task_t *parent, task_t *child)
{
    child->parent = parent;
    list_add_tail(&child->sibling, &parent->children);

    child->next = kernel_task->next;
    kernel_task->next = child;

    task_insert(child);
}

task_t *task_from_image(task_t *parent, struct image *img, const char *name)
{
    if (find_free_slot() < 0)
        return NULL;

    task_t *t = task_alloc();
    if (!t)
        return NULL;

    t->pml4 = img->pml4;
    t->mm = img->mm;
    t->sigterm_handler_rip = img->sigterm;
    task_set_name(t, name);
    task_setup_user_context(t, img);
    task_inherit_fds(t, parent);
    t->pid = new_pid();
    build_kernel_frame(t);

    img->pml4 = NULL;

    task_link(parent, t);
    t->state = TASK_READY;
    return t;
}

task_t *task_create(void *elf_data, size_t elf_size, int parent_pid, char *name, int argc, char **argv, int frames)
{
    uint64_t flags = irq_save();
    task_t *result = NULL;

    task_t *parent = find_by_pid(parent_pid);
    if (!parent)
        parent = current_task;

    struct image img;
    if (image_build(elf_data, elf_size, argc, argv, frames, &img) == 0)
    {
        result = task_from_image(parent, &img, name);
        if (!result)
            image_free(&img);
    }

    irq_restore(flags);
    return result;
}

void task_release(task_t *t)
{
    if (!t)
        return;

    for (int i = 0; i < MAX_FILES_PER_TASK; i++)
    {
        if (t->fd_table[i])
            vfs_close(i, t->fd_table);
    }

    if (t->pml4 && t->pml4 != kernel_pml4_virt)
        vmm_free(t->pml4);
    t->pml4 = NULL;

    task_free_kstack(t);
    kfree(t);
}

static void unlink_from_runqueue(task_t *victim)
{
    task_t *node = kernel_task;
    do
    {
        if (node->next == victim)
        {
            node->next = victim->next;
            return;
        }
        node = node->next;
    } while (node != kernel_task);
}

void task_terminate(task_t *t, int code)
{
    if (!t || t == kernel_task || t->state == TASK_ZOMBIE)
        return;

    t->exit_code = code;
    t->state = TASK_ZOMBIE;
    t->pending_signal = 0;

    for (int i = 0; i < MAX_FILES_PER_TASK; i++)
    {
        if (t->fd_table[i])
            vfs_close(i, t->fd_table);
    }

    list_head_t *node = t->children.next;
    while (node != &t->children)
    {
        list_head_t *following = node->next;
        task_t *child = list_entry(node, task_t, sibling);

        list_del(node);
        child->parent = kernel_task;
        list_add_tail(node, &kernel_task->children);
        node = following;
    }

    if (t->parent && t->parent->state == TASK_BLOCKED)
        t->parent->state = TASK_READY;
}

void task_reap(task_t *t)
{
    if (!t || t == current_task || t == kernel_task || t->state != TASK_ZOMBIE)
        return;

    unlink_from_runqueue(t);

    int slot = find_in_array(t);
    if (slot > 0)
        tasks[slot] = NULL;

    if (t->sibling.next)
        list_del(&t->sibling);

    task_release(t);
}

void task_reap_orphans(void)
{
    uint64_t flags = irq_save();
    for (int i = 1; i < MAX_TASKS; i++)
    {
        task_t *t = tasks[i];
        if (t && t->state == TASK_ZOMBIE && t->parent == kernel_task && t != current_task)
            task_reap(t);
    }
    irq_restore(flags);
}

uint64_t task_sbrk(task_t *t, int64_t increment)
{
    uint64_t old_brk = t->mm.brk;
    if (increment == 0)
        return old_brk;

    uint64_t new_brk;
    if (increment > 0)
    {
        if ((uint64_t)increment > USER_HEAP_MAX || old_brk + (uint64_t)increment > t->mm.brk_start + USER_HEAP_MAX)
            return (uint64_t)-1;
        new_brk = old_brk + (uint64_t)increment;
    }
    else
    {
        uint64_t dec = (uint64_t)(-increment);
        if (dec > old_brk || old_brk - dec < t->mm.brk_start)
            return (uint64_t)-1;
        new_brk = old_brk - dec;
    }

    uintptr_t old_page = (old_brk + PAGE_SIZE - 1) & ~(uintptr_t)(PAGE_SIZE - 1);
    uintptr_t new_page = (new_brk + PAGE_SIZE - 1) & ~(uintptr_t)(PAGE_SIZE - 1);

    if (new_page > old_page)
    {
        uintptr_t va;
        for (va = old_page; va < new_page; va += PAGE_SIZE)
        {
            void *frame = pmm_alloc_frame_zeroed();
            if (!frame || vmm_map_page(t->pml4, va, (uintptr_t)frame, VMM_PRESENT | VMM_WRITE | VMM_USER) != 0)
            {
                if (frame)
                    pmm_free_frame(frame);
                for (uintptr_t undo = old_page; undo < va; undo += PAGE_SIZE)
                {
                    uintptr_t phys;
                    uint64_t fl;
                    if (vmm_resolve(t->pml4, undo, &phys, &fl) == 0)
                    {
                        vmm_unmap_page(t->pml4, undo);
                        pmm_free_frame((void *)(phys & ~(uintptr_t)(PAGE_SIZE - 1)));
                    }
                }
                return (uint64_t)-1;
            }
        }
    }
    else if (new_page < old_page)
    {
        for (uintptr_t va = new_page; va < old_page; va += PAGE_SIZE)
        {
            uintptr_t phys;
            uint64_t fl;
            if (vmm_resolve(t->pml4, va, &phys, &fl) == 0)
            {
                vmm_unmap_page(t->pml4, va);
                pmm_free_frame((void *)(phys & ~(uintptr_t)(PAGE_SIZE - 1)));
            }
        }
    }

    t->mm.brk = new_brk;
    return old_brk;
}
