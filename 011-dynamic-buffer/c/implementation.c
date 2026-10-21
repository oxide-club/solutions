/* Nunca publique a reserva antes do sucesso: realloc pode falhar sem liberar a
 * antiga. */
#include "api.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void buffer_init(Buffer *b, Resize resize) {
  b->data = NULL;
  b->size = 0;
  b->capacity = 0;
  b->resize = resize;
}
int buffer_append(Buffer *b, const void *src, size_t n) {
  if (n > SIZE_MAX - b->size)
    return 0;
  size_t need = b->size + n;
  if (need > b->capacity) {
    size_t cap = b->capacity ? b->capacity : 8;
    while (cap < need) {
      if (cap > SIZE_MAX / 2) {
        cap = need;
        break;
      }
      cap *= 2;
    }
    void *next = b->resize(b->data, cap);
    if (!next)
      return 0;
    b->data = next;
    b->capacity = cap;
  }
  if (n)
    memcpy(b->data + b->size, src, n);
  b->size = need;
  return 1;
}
void buffer_destroy(Buffer *b) {
  free(b->data);
  b->data = NULL;
  b->size = 0;
  b->capacity = 0;
}
