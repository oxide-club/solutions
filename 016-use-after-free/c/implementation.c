/* A propriedade da cópia passa ao chamador, que a libera depois do último uso.
 */
#include "api.h"
#include <stdlib.h>
#include <string.h>
unsigned char *copy_bytes(const unsigned char *src, size_t n) {
  unsigned char *p = malloc(n ? n : 1);
  if (!p)
    return NULL;
  if (n)
    memcpy(p, src, n);
  return p;
}
