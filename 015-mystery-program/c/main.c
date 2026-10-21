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
  if (len > 64)
    fail();
  unsigned char out[64];
  for (size_t i = 0; i < len; i++)
    out[len - 1 - i] = (unsigned char)((data[i] ^ 0x5a) + i);
  output(out, len);
  finish();
  return 0;
}
