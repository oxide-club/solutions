#ifndef OXIDE_API_H
#define OXIDE_API_H
#include <stddef.h>
#include <stdint.h>
typedef void *(*Resize)(void *, size_t);
typedef struct {
  unsigned char *data;
  size_t size, capacity;
  Resize resize;
} Buffer;
void buffer_init(Buffer *, Resize);
int buffer_append(Buffer *, const void *, size_t);
void buffer_destroy(Buffer *);
#endif
