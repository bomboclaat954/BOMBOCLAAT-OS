/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
/*
 Maybe this file shouldn't have my copyrights, most of it was
 copied from other open source projects like Linux or FreeBSD...
 */
#include <bomboclaat/globals.h>
#ifndef TYPES_H
#define TYPES_H

struct list_head
{
    struct list_head *next, *prev;
} typedef list_head_t;

typedef int pid_t;

#define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) ); })

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each_entry(pos, head, member)                 \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head);                               \
         pos = list_entry(pos->member.next, typeof(*pos), member))

static inline void INIT_LIST_HEAD(list_head_t *list)
{
    list->next = list;
    list->prev = list;
}

static inline void list_add_tail(list_head_t *new_node, list_head_t *head)
{
    list_head_t *prev = head->prev;

    new_node->next = head;
    new_node->prev = prev;
    prev->next = new_node;
    head->prev = new_node;
}

static inline void list_del(list_head_t *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = NULL;
    entry->prev = NULL;
}

#endif
