#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int fail(void) {
  fputs("entrada, argumentos ou I/O invalidos\n", stderr);
  return 1;
}
static void setup(void) {
#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif
}
static int finish(void) {
  return ferror(stdin) || ferror(stdout) || fflush(stdout) == EOF ? fail() : 0;
}
#include "ppm.h"
int main(int argc, char **argv) {
  (void)argv;
  setup();
  if (argc != 1)
    return fail();
  Image in, out;
  if (!ppm_read(stdin, &in))
    return fail();
  out = in;
  out.pixels = malloc(in.w * in.h * 3);
  if (!out.pixels) {
    free(in.pixels);
    return fail();
  }
  for (size_t y = 0; y < in.h; y++)
    for (size_t x = 0; x < in.w; x++) {
      /* Cada linha tem largura w; o stride não é a altura. */
      size_t src = (y * in.w + x) * 3, dst = (y * in.w + x) * 3;
      memcpy(out.pixels + dst, in.pixels + src, 3);
    }
  int ok = ppm_write(stdout, &out);
  free(in.pixels);
  free(out.pixels);
  return ok ? finish() : fail();
}
