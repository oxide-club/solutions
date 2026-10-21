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
static int nibble(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}
static int decode(const char *s, unsigned char *p, size_t *n) {
  size_t len = strlen(s);
  *n = 0;
  if (!strcmp(s, "-"))
    return 1;
  if (len % 2)
    return 0;
  for (size_t i = 0; i < len; i += 2) {
    int a = nibble(s[i]), b = nibble(s[i + 1]);
    if (a < 0 || b < 0)
      return 0;
    p[(*n)++] = (unsigned char)(a * 16 + b);
  }
  return 1;
}
static void dump(const unsigned char *p, size_t n, size_t c) {
  printf("%zu/%zu:", n, c);
  for (size_t i = 0; i < n; i++)
    printf("%02x", p[i]);
  putchar('\n');
}
int main(int argc, char **argv) {
  (void)argv;
  if (argc != 1)
    return bad();
  Buffer b;
  char cmd[32], text[4097];
  unsigned char bytes[2048];
  size_t n;
  unsigned char region[65536];
  size_t cap;
  if (scanf("%zu", &cap) != 1 || cap > sizeof region)
    return bad();
  buffer_init(&b, region, cap);
  while (scanf("%31s", cmd) == 1) {
    if (!strcmp(cmd, "a")) {
      if (scanf("%4096s", text) != 1 || !decode(text, bytes, &n))
        return bad();
      puts(buffer_append(&b, bytes, n) ? "ok" : "fail");
    } else if (!strcmp(cmd, "s"))
      dump(b.data, b.size, b.capacity);
    else if (!strcmp(cmd, "o"))
      puts(buffer_append(&b, NULL, SIZE_MAX) ? "ok" : "fail");
    else if (!strcmp(cmd, "c"))
      buffer_clear(&b);
    else
      return bad();
  }
  return finish();
}
