#include <stddef.h>
#include <tasks/tasks.h>

__attribute__((naked)) void cpu_switch_context(task_t *prev, task_t *next)
{
    asm volatile(
        "push %%rbp\n"
        "push %%rbx\n"
        "push %%r12\n"
        "push %%r13\n"
        "push %%r14\n"
        "push %%r15\n"
        "mov %%rsp, %c[off](%%rdi)\n"
        "mov %c[off](%%rsi), %%rsp\n"
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%rbx\n"
        "pop %%rbp\n"
        "ret\n"
        :
        : [off] "i"(offsetof(task_t, kernel_rsp))
        : "memory");
}

__attribute__((naked, noreturn)) void enter_new_context(uintptr_t kernel_rsp)
{
    asm volatile(
        "mov %%rdi, %%rsp\n"
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%rbx\n"
        "pop %%rbp\n"
        "ret\n"
        :
        :
        : "memory");
}
