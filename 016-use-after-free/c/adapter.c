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
  unsigned char input[65537];
  size_t n = fread(input, 1, sizeof input, stdin);
  if (n > 65536 || ferror(stdin))
    return bad();
  unsigned char *owned = copy_bytes(input, n);
  if (!owned)
    return bad();
  for (size_t i = 0; i < n; i++)
    printf("%02x", owned[i]);
  putchar('\n');
  free(owned);
  return finish();
}
