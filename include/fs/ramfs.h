/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ramfs_H
#define ramfs_H
#include <stdint.h>

struct ramfs_dir
{
    char *name;
    struct ramfs_dir *parent_dir; // NULL for root dir
    struct ramfs_dir **child_dirs;
    int child_dirs_count;
    struct ramfs_file **files;
    int files_count;
} __attribute__((packed)) typedef ramfs_dir_t;

struct ramfs_file
{
    char *name;
    ramfs_dir_t *dir;
    uint64_t size;
    uint8_t *content;
} typedef ramfs_file_t;

struct ramfs
{
    ramfs_dir_t **dirs;
    ramfs_file_t **files;
} typedef ramfs_t;

void ramfs_ls();
ramfs_t *init_ramfs();

void ramfs_ls();
uint8_t *ramfs_read_file(ramfs_file_t *f);
ramfs_file_t *ramfs_file_append(ramfs_file_t *f, uint8_t *data, int size);
ramfs_file_t *ramfs_create_file(ramfs_dir_t *dir, uint8_t *data, int data_size, char *name);
ramfs_dir_t *ramfs_create_dir(char *name, ramfs_dir_t *parent);
ramfs_t *init_ramfs();

#endif
