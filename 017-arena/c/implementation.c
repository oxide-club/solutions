/* Base alinhada a 16; used <= capacity; subtrações evitam overflow das somas.
 */
#include "api.h"
#include <stdint.h>
void arena_init(Arena *a, void *base, size_t capacity) {
  a->base = base;
  a->used = 0;
  a->capacity = capacity;
}
void *arena_alloc(Arena *a, size_t n, size_t alignment) {
  if (!n || !alignment || alignment > 16 || (alignment & (alignment - 1)))
    return NULL;
  size_t padding = (alignment - (a->used & (alignment - 1))) & (alignment - 1);
  if (padding > a->capacity - a->used)
    return NULL;
  size_t start = a->used + padding;
  if (n > a->capacity - start)
    return NULL;
  a->used = start + n;
  return a->base + start;
}
void arena_reset(Arena *a) { a->used = 0; }
