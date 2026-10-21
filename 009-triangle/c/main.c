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
#include <ctype.h>
typedef struct {
  int64_t x, y, c[3];
} Vertex;
/* Limite do acumulador evita overflow antes de aplicar limites do contrato. */
static int integer(int64_t *v) {
  int c;
  do {
    c = getchar();
  } while (c != EOF && isspace((unsigned char)c));
  int neg = c == '-';
  if (neg)
    c = getchar();
  if (c < '0' || c > '9')
    return 0;
  int64_t n = 0;
  do {
    if (n > 1000000)
      return 0;
    n = n * 10 + c - '0';
    c = getchar();
  } while (c >= '0' && c <= '9');
  if (c != EOF && !isspace((unsigned char)c))
    return 0;
  *v = neg ? -n : n;
  return 1;
}
/* A amostra chega dobrada: evita arredondamento em testes de cobertura. */
static int64_t edge(Vertex a, Vertex b, int64_t x, int64_t y) {
  return (b.x - a.x) * (y - 2 * a.y) - (b.y - a.y) * (x - 2 * a.x);
}
/* Apenas uma das duas orientacoes de cada borda possui os empates. */
static int inside(int64_t e, Vertex a, Vertex b) {
  return e > 0 || (e == 0 && (b.y < a.y || (b.y == a.y && b.x > a.x)));
}
int main(int argc, char **argv) {
  (void)argv;
  setup();
  if (argc != 1)
    return fail();
  int64_t w, h, n;
  if (!integer(&w) || !integer(&h) || !integer(&n) || w < 1 || h < 1 ||
      w > 256 || h > 256 || n < 0 || n > 1000)
    return fail();
  Image out = {(size_t)w, (size_t)h, NULL};
  out.pixels = calloc((size_t)(w * h), 3);
  if (!out.pixels)
    return fail();
  for (int64_t t = 0; t < n; t++) {
    Vertex v[3];
    for (int j = 0; j < 3; j++) {
      if (!integer(&v[j].x) || !integer(&v[j].y) || v[j].x < -1024 ||
          v[j].x > 1024 || v[j].y < -1024 || v[j].y > 1024) {
        free(out.pixels);
        return fail();
      }
      for (int k = 0; k < 3; k++)
        if (!integer(&v[j].c[k]) || v[j].c[k] < 0 || v[j].c[k] > 255) {
          free(out.pixels);
          return fail();
        }
    }
    /* Normalizar winding tambem troca as cores associadas aos vertices. */
    int64_t area = edge(v[0], v[1], 2 * v[2].x, 2 * v[2].y);
    if (!area)
      continue;
    if (area < 0) {
      Vertex tmp = v[1];
      v[1] = v[2];
      v[2] = tmp;
      area = -area;
    }
    for (int64_t y = 0; y < h; y++)
      for (int64_t x = 0; x < w; x++) {
        int64_t e[3] = {edge(v[1], v[2], 2 * x + 1, 2 * y + 1),
                        edge(v[2], v[0], 2 * x + 1, 2 * y + 1),
                        edge(v[0], v[1], 2 * x + 1, 2 * y + 1)};
        if (!inside(e[0], v[1], v[2]) || !inside(e[1], v[2], v[0]) ||
            !inside(e[2], v[0], v[1]))
          continue;
        for (int k = 0; k < 3; k++) {
          /* Pesos inteiros / area: arredondamento nearest, empate para cima. */
          int64_t sum = e[0] * v[0].c[k] + e[1] * v[1].c[k] + e[2] * v[2].c[k];
          out.pixels[3 * (size_t)(y * w + x) + (size_t)k] =
              (unsigned char)((2 * sum + area) / (2 * area));
        }
      }
  }
  int ch;
  do {
    ch = getchar();
  } while (ch != EOF && isspace((unsigned char)ch));
  if (ch != EOF) {
    free(out.pixels);
    return fail();
  }
  int ok = ppm_write(stdout, &out);
  free(out.pixels);
  return ok ? finish() : fail();
}
