/* Fator de carga <= 1/2 garante um slot vazio para terminar a sondagem. */
#include "api.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static size_t slot(const Pair *p, size_t cap, int32_t key) {
  size_t i = (uint32_t)key & (cap - 1);
  while (p[i].occupied && p[i].key != key)
    i = (i + 1) & (cap - 1);
  return i;
}
void table_init(Table *t, Allocate allocate) {
  t->pairs = NULL;
  t->count = 0;
  t->capacity = 0;
  t->allocate = allocate;
}
int table_put(Table *t, int32_t k, int32_t v) {
  if (t->capacity) {
    size_t i = slot(t->pairs, t->capacity, k);
    if (t->pairs[i].occupied) {
      t->pairs[i].value = v;
      return 1;
    }
  }
  if (!t->capacity || t->count >= t->capacity / 2) {
    if (t->capacity > SIZE_MAX / 2)
      return 0;
    size_t cap = t->capacity ? t->capacity * 2 : 8;
    if (cap > SIZE_MAX / sizeof(Pair))
      return 0;
    Pair *p = t->allocate(cap * sizeof *p);
    if (!p)
      return 0;
    memset(p, 0, cap * sizeof *p);
    for (size_t j = 0; j < t->capacity; j++)
      if (t->pairs[j].occupied)
        p[slot(p, cap, t->pairs[j].key)] = t->pairs[j];
    free(t->pairs);
    t->pairs = p;
    t->capacity = cap;
  }
  size_t i = slot(t->pairs, t->capacity, k);
  t->pairs[i] = (Pair){k, v, 1};
  t->count++;
  return 1;
}
int table_get(const Table *t, int32_t k, int32_t *v) {
  if (!t->capacity)
    return 0;
  size_t i = slot(t->pairs, t->capacity, k);
  if (!t->pairs[i].occupied)
    return 0;
  *v = t->pairs[i].value;
  return 1;
}
void table_destroy(Table *t) {
  free(t->pairs);
  t->pairs = NULL;
  t->count = 0;
  t->capacity = 0;
}
