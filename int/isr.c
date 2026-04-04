#include <int.h>
#include <api.h>
#include <screen.h>
#include <string.h>

void exception_handler(registers_t *r)
{
    switch (r->int_no)
    {
    case 0:
        panic("CPU-EXC: division by zero", r, 1);
        break;
    case 1:
        panic("CPU-EXC: debug", r, 1);
        break;
    case 2:
        panic("CPU-EXC: non-maskable interrupt", r, 1);
        break;
    case 3:
        panic("CPU-EXC: breakpoint", r, 1);
        break;
    case 4:
        panic("CPU-EXC: overflow", r, 1);
        break;
    case 5:
        panic("CPU-EXC: bound range exceeded", r, 1);
        break;
    case 6:
        panic("CPU-EXC: invalid opcode", r, 1);
        break;
    case 7:
        panic("CPU-EXC: devide not available", r, 1);
        break;
    case 8:
        panic("CPU-EXC: double fault", r, 1);
        break;
    case 10:
        panic("CPU-EXC: invalic TSS", r, 1);
        break;
    case 11:
        panic("CPU-EXC: segment not present", r, 1);
        break;
    case 12:
        panic("CPU-EXC: stack-segment fault", r, 1);
        break;
    case 13:
        panic("CPU-EXC: general protection fault", r, 1);
        break;
    case 14:
        /*uint32_t fault_addr;
        __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));*/
        panic("CPU-EXC: page fault", r, 1);
        break;
    case 16:
        panic("CPU-EXC: x87 float", r, 1);
        break;
    case 17:
        panic("CPU-EXC: alignment check", r, 1);
        break;
    case 18:
        panic("CPU-EXC: machine check", r, 1);
        break;
    case 19:
        panic("CPU-EXC: SIMD float", r, 1);
        break;
    case 20:
        panic("CPU-EXC: virtualization", r, 1);
        break;
    case 21:
        panic("CPU-EXC: control protection", r, 1);
        break;
    case 28:
        panic("CPU-EXC: hypervisor injection", r, 1);
        break;
    case 29:
        panic("CPU-EXC: VMM communication", r, 1);
        break;
    case 30:
        panic("CPU-EXC: security", r, 1);
        break;
    default:
        panic("CPU-EXC: unknown", r, 1);
        break;
    }
}
