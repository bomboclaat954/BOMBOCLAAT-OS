#include <memory/kmalloc.h>
#include <cpp/cppdef.hpp>

void *operator new(size_t size)
{
    return kmalloc(size);
}

void operator delete(void *p, size_t size)
{
    kfree(p);
}
