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
int main(int argc, char **argv) {
#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif
  (void)argv;
  if (argc != 1)
    fail();
  readall();
  if (len < 8 || memcmp(data, "PX\1\0", 4))
    fail();
  unsigned w = ((unsigned)data[4] << 8) | data[5],
           h = ((unsigned)data[6] << 8) | data[7];
  if (!w || !h || w > 64 || h > 64 || len != 8 + 3 * w * h)
    fail();
  printf("P6\n%u %u\n255\n", w, h);
  for (unsigned y = 0; y < h; y++)
    for (unsigned x = 0; x < w; x++) {
      size_t p = 8 + 3 * (x * h + y);
      unsigned char rgb[] = {data[p + 2], data[p + 1], data[p]};
      output(rgb, 3);
    }
  finish();
  return 0;
}
