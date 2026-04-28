/*
    BOMBOCLAAT-OS KMALLOC
*/
#include <memory/kmalloc.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGNMENT 8u
#define ALIGN_UP(x, a) ((((uintptr_t)(x)) + ((uintptr_t)(a) - 1u)) & ~((uintptr_t)((a) - 1u)))

#define KMALLOC_ALIGN_MAGIC 0xDEADBEEFu

typedef struct kmem_block
{
    size_t size;
    struct kmem_block *next;
    uint8_t free;

} kmem_block_t;

static kmem_block_t *free_list = NULL;
static uint8_t *heap_start = NULL;
static uint8_t *heap_end = NULL;

static void split_block(kmem_block_t *block, size_t size);

void heap_init(void *start, size_t size)
{
    if (!start || size <= sizeof(kmem_block_t) + ALIGNMENT)
        return;

    uintptr_t s = (uintptr_t)start;
    s = ALIGN_UP(s, ALIGNMENT);
    heap_start = (uint8_t *)s;
    heap_end = (uint8_t *)((uintptr_t)start + size);

    free_list = (kmem_block_t *)heap_start;
    free_list->size = (size_t)(heap_end - heap_start) - sizeof(kmem_block_t);
    free_list->next = NULL;
    free_list->free = 1;
}

static void split_block(kmem_block_t *block, size_t size)
{
    if (!block)
        return;

    if (block->size < size + sizeof(kmem_block_t) + ALIGNMENT)
        return;

    uint8_t *new_block_ptr = (uint8_t *)block + sizeof(kmem_block_t) + size;
    kmem_block_t *new_block = (kmem_block_t *)new_block_ptr;

    new_block->size = block->size - size - sizeof(kmem_block_t);
    new_block->next = block->next;
    new_block->free = 1;

    block->next = new_block;
    block->size = size;
}

void *kmalloc(size_t size)
{
    if (size == 0)
        return NULL;
    size = (size_t)ALIGN_UP(size, ALIGNMENT);

    kmem_block_t *cur = free_list;

    while (cur)
    {
        if (cur->free && cur->size >= size)
        {
            split_block(cur, size);
            cur->free = 0;
            void *userptr = (void *)((uint8_t *)cur + sizeof(kmem_block_t));
            return userptr;
        }

        cur = cur->next;
    }

    return NULL;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;

    uintptr_t min_magic_offset = sizeof(uint32_t) + sizeof(uintptr_t);
    uint8_t *p = (uint8_t *)ptr;

    if (heap_start != NULL && (p - min_magic_offset) >= heap_start)
    {
        uint8_t *magic_addr = p - min_magic_offset;
        uint32_t maybe_magic = *((uint32_t *)magic_addr);
        if (maybe_magic == KMALLOC_ALIGN_MAGIC)
        {

            uintptr_t *raw_addr = (uintptr_t *)(p - sizeof(uintptr_t));
            void *raw_ptr = (void *)(*raw_addr);

            if ((uint8_t *)raw_ptr >= heap_start && (uint8_t *)raw_ptr < heap_end)
            {
                ptr = raw_ptr;
            }
        }
    }

    kmem_block_t *block = (kmem_block_t *)((uint8_t *)ptr - sizeof(kmem_block_t));
    block->free = 1;

    kmem_block_t *cur = free_list;
    while (cur && cur->next)
    {
        if (cur->free && cur->next->free)
        {

            uint8_t *cur_end = (uint8_t *)cur + sizeof(kmem_block_t) + cur->size;
            if (cur_end == (uint8_t *)cur->next)
            {

                cur->size = cur->size + sizeof(kmem_block_t) + cur->next->size;
                cur->next = cur->next->next;

                continue;
            }
        }
        cur = cur->next;
    }
}

void *kmalloc_aligned(size_t size, size_t alignment)
{
    if (size == 0)
        return NULL;
    if (alignment <= ALIGNMENT)
        return kmalloc(size);

    size_t extra = alignment + sizeof(uint32_t) + sizeof(uintptr_t);
    size_t real_size = size + extra;
    void *raw = kmalloc(real_size);
    if (!raw)
        return NULL;

    uintptr_t p = (uintptr_t)raw;
    uintptr_t offset = (uintptr_t)(sizeof(uint32_t) + sizeof(uintptr_t));
    uintptr_t aligned = ALIGN_UP(p + offset, alignment);

    uint8_t *magic_addr = (uint8_t *)(aligned - offset);
    uint32_t *magic = (uint32_t *)magic_addr;
    uintptr_t *raw_addr = (uintptr_t *)(aligned - sizeof(uintptr_t));

    *magic = KMALLOC_ALIGN_MAGIC;
    *raw_addr = (uintptr_t)raw;

    return (void *)aligned;
}
