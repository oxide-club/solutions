#ifndef OXIDE_API_H
#define OXIDE_API_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
  unsigned char *data;
  size_t size, capacity;
} Buffer;
void buffer_init(Buffer *, unsigned char *, size_t);
int buffer_append(Buffer *, const void *, size_t);
void buffer_clear(Buffer *);
#endif
