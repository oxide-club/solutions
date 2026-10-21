/* Descritores cobrem a região em ordem; livres adjacentes são unidos após free.
 */
#include "api.h"
#include <stdint.h>
void pool_init(Pool *p, void *base, size_t capacity) {
  p->base = base;
  p->capacity = capacity - capacity % 8;
  p->count = p->capacity ? 1 : 0;
  if (p->count)
    p->blocks[0] = (Block){0, p->capacity, 0};
}
void *pool_alloc(Pool *p, size_t n) {
  if (!n || n > SIZE_MAX - 7)
    return NULL;
  n = (n + 7) & ~(size_t)7;
  for (size_t i = 0; i < p->count; i++) {
    Block *b = &p->blocks[i];
    if (b->used || b->size < n)
      continue;
    if (b->size > n) {
      if (p->count == BLOCK_LIMIT)
        return NULL;
      for (size_t j = p->count; j > i + 1; j--)
        p->blocks[j] = p->blocks[j - 1];
      p->blocks[i + 1] = (Block){b->offset + n, b->size - n, 0};
      p->count++;
      b->size = n;
    }
    b->used = 1;
    return p->base + b->offset;
  }
  return NULL;
}
void pool_free(Pool *p, void *ptr) {
  size_t off = (size_t)((unsigned char *)ptr - p->base), i = 0;
  while (i < p->count && p->blocks[i].offset != off)
    i++;
  if (i == p->count)
    return;
  p->blocks[i].used = 0;
  for (i = 0; i + 1 < p->count;) {
    if (!p->blocks[i].used && !p->blocks[i + 1].used) {
      p->blocks[i].size += p->blocks[i + 1].size;
      for (size_t j = i + 1; j + 1 < p->count; j++)
        p->blocks[j] = p->blocks[j + 1];
      p->count--;
    } else
      i++;
  }
}
