// some necessary definitions for C++ code
#ifndef CPPDEF_H
#define CPPDEF_H
#include <stddef.h>

void *operator new(size_t size);
void operator delete(void *p, size_t size);

#endif
