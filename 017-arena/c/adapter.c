#include "api.h"
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int bad(void) {
  fputs("entrada ou I/O inválido\n", stderr);
  return 1;
}
static int finish(void) {
  return ferror(stdin) || fflush(stdout) == EOF ? bad() : 0;
}
int main(int argc, char **argv) {
  (void)argv;
  if (argc != 1)
    return bad();
  _Alignas(16) unsigned char region[65536];
  size_t cap, n, align;
  Arena a;
  char cmd[32];
  if (scanf("%zu", &cap) != 1 || cap > sizeof region)
    return bad();
  arena_init(&a, region, cap);
  while (scanf("%31s", cmd) == 1) {
    if (!strcmp(cmd, "a") || !strcmp(cmd, "o")) {
      if (cmd[0] == 'o') {
        n = SIZE_MAX;
        align = 16;
      } else if (scanf("%zu %zu", &n, &align) != 2)
        return bad();
      void *p = arena_alloc(&a, n, align);
      if (p)
        printf("%zu\n", (size_t)((unsigned char *)p - region));
      else
        puts("fail");
    } else if (!strcmp(cmd, "s"))
      printf("%zu/%zu\n", a.used, a.capacity);
    else if (!strcmp(cmd, "r"))
      arena_reset(&a);
    else
      return bad();
  }
  return finish();
}
