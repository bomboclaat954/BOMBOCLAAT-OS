#include <int.h>

void exception_handler(registers_t *r)
{
    __asm__ volatile("cli; hlt");
}
