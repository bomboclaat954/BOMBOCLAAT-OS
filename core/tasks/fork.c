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

#include <bomboclaat/types.h>
#include <tasks/tasks.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>

task_t *create_child(task_t *parent)
{
    if (!parent)
        return NULL;

    task_t *new = (task_t *)kmalloc(sizeof(task_t));
    if (!new)
        return NULL;

    memcpy((uint8_t *)new, (uint8_t *)parent, sizeof(task_t));

    new->parent = parent;
    new->pid = new_pid();
    new->state = TASK_NEW;

    INIT_LIST_HEAD(&new->children);
    list_add_tail(&new->sibling, &parent->children);

    return new;
}

int fork()
{
    extern task_t *current_task;

    if (!current_task)
        return -1;

    task_t *new = create_child(current_task);
    if (!new)
        return -1;

    if (task_insert(new) < 0)
    {
        list_del(&new->sibling);
        kfree(new);
        return -1;
    }

    return new->pid;
}

// TODO: vfork and other useless ones to make it look "professional"
