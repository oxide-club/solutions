#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char data[65537];
static size_t len;
static void fail(void) {
  fputs("entrada ou operação inválida\n", stderr);
  exit(1);
}
static void output(const void *p, size_t n) {
  if (fwrite(p, 1, n, stdout) != n)
    fail();
}
static void finish(void) {
  if (fflush(stdout) || ferror(stdout))
    fail();
}
/* Um byte extra distingue o limite aceito de uma entrada excessiva. */
static void readall(void) {
  len = fread(data, 1, sizeof(data), stdin);
  if (ferror(stdin) || len > 65536)
    fail();
}
static unsigned u16(size_t p) { return data[p] | ((unsigned)data[p + 1] << 8); }
int main(int argc, char **argv) {
#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif
  (void)argv;
  if (argc != 1)
    fail();
  readall();
  if (len < 6 || memcmp(data, "REC1", 4))
    fail();
  unsigned count = u16(4);
  size_t p = 6;
  for (unsigned i = 0; i < count; i++) {
    if (len - p < 3)
      fail();
    unsigned n = u16(p + 1);
    p += 3;
    if (n > len - p)
      fail();
    p += n;
  }
  if (p != len)
    fail();
  printf("count=%u\n", count);
  p = 6;
  for (unsigned i = 0; i < count; i++) {
    unsigned n = u16(p + 1);
    printf("offset=%zu type=%u length=%u data=", p, data[p], n);
    p += 3;
    for (unsigned j = 0; j < n; j++)
      printf("%02x", data[p++]);
    putchar('\n');
  }
  output("", 0);
  finish();
  return 0;
}
