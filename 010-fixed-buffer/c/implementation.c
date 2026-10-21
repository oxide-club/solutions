/* Invariante: size <= capacity; a memória continua pertencendo ao chamador. */
#include "api.h"
#include <string.h>
void buffer_init(Buffer *b, unsigned char *p, size_t n) {
  b->data = p;
  b->size = 0;
  b->capacity = n;
}
int buffer_append(Buffer *b, const void *p, size_t n) {
  if (n > b->capacity - b->size)
    return 0;
  if (n)
    memcpy(b->data + b->size, p, n);
  b->size += n;
  return 1;
}
void buffer_clear(Buffer *b) { b->size = 0; }
