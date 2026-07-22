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

#include <stddef.h>
#include <stdint.h>
#include <malloc.h>

block_header_t *free_list_start = NULL;

void *sbrk(size_t increment)
{
    uint64_t result;
    asm volatile(
        "int $0x80"
        : "=a"(result)
        : "a"(9), "D"(increment)
        : "memory");
    return (void *)result;
}

block_header_t *find_free_block(size_t size)
{
    block_header_t *current = free_list_start;
    while (current != NULL)
    {
        if (current->is_free && current->size >= size)
            return current;
        current = current->next;
    }
    return NULL;
}

void *malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size = (size + 7) & ~7;

    block_header_t *block = find_free_block(size);

    if (block != NULL)
    {
        block->is_free = false;
        return (void *)(block + 1);
    }

    size_t total_size = HEADER_SIZE + size;
    block = (block_header_t *)sbrk(total_size);

    if (block == (void *)-1)
        return NULL;

    block->size = size;
    block->is_free = false;
    block->next = NULL;

    if (free_list_start == NULL)
        free_list_start = block;
    else
    {
        block_header_t *current = free_list_start;
        while (current->next != NULL)
            current = current->next;
        current->next = block;
    }

    return (void *)(block + 1);
}

void coalesce_free_blocks(void)
{
    block_header_t *current = free_list_start;
    while (current != NULL && current->next != NULL)
    {
        if (current->is_free && current->next->is_free)
        {
            current->size += HEADER_SIZE + current->next->size;
            current->next = current->next->next;
        }
        else
            current = current->next;
    }
}

void free(void *ptr)
{
    if (ptr == NULL)
        return;
    block_header_t *block = (block_header_t *)ptr - 1;
    block->is_free = true;
    coalesce_free_blocks();
}
