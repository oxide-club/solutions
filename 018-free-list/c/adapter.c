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
  void *handles[8192] = {0};
  size_t cap, n, serial = 0;
  Pool p;
  char cmd[32];
  if (scanf("%zu", &cap) != 1 || cap > sizeof region)
    return bad();
  pool_init(&p, region, cap);
  while (scanf("%31s", cmd) == 1) {
    if (!strcmp(cmd, "a") || !strcmp(cmd, "o")) {
      if (cmd[0] == 'o')
        n = SIZE_MAX;
      else if (scanf("%zu", &n) != 1)
        return bad();
      if (serial >= 8192)
        return bad();
      void *ptr = pool_alloc(&p, n);
      if (!ptr)
        puts("fail");
      else {
        handles[serial] = ptr;
        printf("%zu:%zu\n", serial, (size_t)((unsigned char *)ptr - region));
        serial++;
      }
    } else if (!strcmp(cmd, "f")) {
      if (scanf("%zu", &n) != 1 || n >= serial || !handles[n])
        return bad();
      pool_free(&p, handles[n]);
      handles[n] = NULL;
    } else if (!strcmp(cmd, "m")) {
      if (!p.count)
        puts("empty");
      else {
        for (size_t i = 0; i < p.count; i++)
          printf("%s%zu:%zu:%c", i ? " " : "", p.blocks[i].offset,
                 p.blocks[i].size, p.blocks[i].used ? 'U' : 'F');
        putchar('\n');
      }
    } else
      return bad();
  }
  return finish();
}
