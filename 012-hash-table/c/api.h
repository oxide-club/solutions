#ifndef OXIDE_API_H
#define OXIDE_API_H
#include <stddef.h>
#include <stdint.h>
typedef void *(*Allocate)(size_t);
typedef struct {
  int32_t key, value;
  int occupied;
} Pair;
typedef struct {
  Pair *pairs;
  size_t count, capacity;
  Allocate allocate;
} Table;
void table_init(Table *, Allocate);
int table_put(Table *, int32_t, int32_t);
int table_get(const Table *, int32_t, int32_t *);
void table_destroy(Table *);
#endif
