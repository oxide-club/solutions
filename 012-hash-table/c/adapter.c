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
static int fail_next;
static void *allocate(size_t n) {
  if (fail_next) {
    fail_next = 0;
    return NULL;
  }
  return malloc(n);
}
int main(int argc, char **argv) {
  (void)argv;
  if (argc != 1)
    return bad();
  Table t;
  table_init(&t, allocate);
  char cmd[32];
  int32_t k, v;
  while (scanf("%31s", cmd) == 1) {
    if (!strcmp(cmd, "p")) {
      if (scanf("%" SCNd32 " %" SCNd32, &k, &v) != 2)
        return bad();
      puts(table_put(&t, k, v) ? "ok" : "fail");
    } else if (!strcmp(cmd, "g")) {
      if (scanf("%" SCNd32, &k) != 1)
        return bad();
      if (table_get(&t, k, &v))
        printf("%" PRId32 "\n", v);
      else
        puts("missing");
    } else if (!strcmp(cmd, "s"))
      printf("%zu/%zu\n", t.count, t.capacity);
    else if (!strcmp(cmd, "f"))
      fail_next = 1;
    else
      return bad();
  }
  table_destroy(&t);
  return finish();
}
