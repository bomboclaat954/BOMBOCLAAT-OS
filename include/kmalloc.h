#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

void heap_init(void *start, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kmalloc_aligned(size_t size, size_t alignment);

#endif
