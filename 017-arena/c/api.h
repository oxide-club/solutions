#ifndef OXIDE_API_H
#define OXIDE_API_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
  unsigned char *base;
  size_t used, capacity;
} Arena;
void arena_init(Arena *, void *, size_t);
void *arena_alloc(Arena *, size_t, size_t);
void arena_reset(Arena *);
#endif
