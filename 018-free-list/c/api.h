#ifndef OXIDE_API_H
#define OXIDE_API_H
#include <stddef.h>
#include <stdint.h>
#define BLOCK_LIMIT 1024
#define BLOCK_ALIGNMENT 8
typedef struct {
  size_t offset, size;
  int used;
} Block;
typedef struct {
  unsigned char *base;
  size_t capacity, count;
  Block blocks[BLOCK_LIMIT];
} Pool;
void pool_init(Pool *, void *, size_t);
void *pool_alloc(Pool *, size_t);
void pool_free(Pool *, void *);
#endif
