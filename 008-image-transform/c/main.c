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
static int number(const char *s, size_t *v) {
  size_t n = 0;
  if (!*s)
    return 0;
  while (*s) {
    if (*s < '0' || *s > '9' || n > 256)
      return 0;
    n = n * 10 + (unsigned)(*s++ - '0');
  }
  if (n > 256)
    return 0;
  *v = n;
  return 1;
}
int main(int argc, char **argv) {
  setup();
  int rotate = argc == 2 && !strcmp(argv[1], "rotate");
  size_t x0 = 0, y0 = 0, w = 0, h = 0;
  if (!rotate && (argc != 6 || strcmp(argv[1], "crop") ||
                  !number(argv[2], &x0) || !number(argv[3], &y0) ||
                  !number(argv[4], &w) || !number(argv[5], &h) || !w || !h))
    return fail();
  Image in, out;
  if (!ppm_read(stdin, &in))
    return fail();
  if (rotate) {
    w = in.h;
    h = in.w;
  } else if (x0 > in.w || y0 > in.h || w > in.w - x0 || h > in.h - y0) {
    free(in.pixels);
    return fail();
  }
  out.w = w;
  out.h = h;
  out.pixels = malloc(w * h * 3);
  if (!out.pixels) {
    free(in.pixels);
    return fail();
  }
  for (size_t y = 0; y < h; y++)
    for (size_t x = 0; x < w; x++) {
      /* Mapeamento inverso: cada destino escolhe exatamente uma origem. */
      size_t sx = rotate ? y : x + x0, sy = rotate ? in.h - 1 - x : y + y0;
      memcpy(out.pixels + 3 * (y * w + x), in.pixels + 3 * (sy * in.w + sx), 3);
    }
  int ok = ppm_write(stdout, &out);
  free(in.pixels);
  free(out.pixels);
  return ok ? finish() : fail();
}
