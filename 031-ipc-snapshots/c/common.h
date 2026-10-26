/* SPDX-License-Identifier: MIT */
#ifndef OX_COMMON_H
#define OX_COMMON_H

#include "region.h"
#include <stddef.h>
#include <time.h>

#define OX_LINE 256
#define OX_LIMIT UINT64_C(1000000)

struct ox_state {
    uint64_t generation, counter, counter_xor;
};

struct timespec ox_deadline(void);
int ox_wait(int fd, short events, const struct timespec *deadline);
int ox_write(int fd, const void *data, size_t length, const struct timespec *deadline);
/* 0: linha completa; -1: recurso/EOF; -2: prazo; -3: framing. */
int ox_line(int fd, char line[OX_LINE + 1], const struct timespec *deadline);
int ox_send(int fd, const char *line, int resource, const struct timespec *deadline);
int ox_hello(int fd, char line[OX_LINE + 1], int *resource, const struct timespec *deadline);
int ox_fields(char *line, char **fields, size_t capacity);
int ox_uint(const char *text, uint64_t *value);
/* 0: ownership recebido; ETIMEDOUT ou outro código: sem ownership. */
int ox_lock(struct ox_region *region, const struct timespec *deadline);
int ox_valid(const struct ox_region *region);
struct ox_state ox_copy(const struct ox_region *region);

#endif
