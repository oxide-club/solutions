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
static int hex(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}
int main(int argc, char **argv) {
  setup();
  if (argc != 2)
    return fail();
  size_t len = strlen(argv[1]);
  if (!len || len % 2 || len > 2097152)
    return fail();
  size_t m = len / 2;
  unsigned char *p = malloc(m);
  size_t *pi = calloc(m, sizeof *pi);
  if (!p || !pi) {
    free(p);
    free(pi);
    return fail();
  }
  for (size_t i = 0; i < m; i++) {
    int a = hex(argv[1][2 * i]), b = hex(argv[1][2 * i + 1]);
    if (a < 0 || b < 0) {
      free(p);
      free(pi);
      return fail();
    }
    p[i] = (unsigned char)(a * 16 + b);
  }
  /* Prefixos próprios: cada recuo reutiliza informação já calculada. */
  for (size_t i = 1, j = 0; i < m; i++) {
    while (j && p[i] != p[j])
      j = pi[j - 1];
    if (p[i] == p[j])
      j++;
    pi[i] = j;
  }
  unsigned char buf[4096];
  size_t n, j = 0;
  uint64_t pos = 0;
  while ((n = fread(buf, 1, sizeof buf, stdin)))
    for (size_t i = 0; i < n; i++) {
      while (j && buf[i] != p[j])
        j = pi[j - 1];
      if (buf[i] == p[j])
        j++;
      if (j == m) {
        printf("%llu\n", (unsigned long long)(pos + 1 - m));
        j = pi[j - 1];
      }
      if (pos == UINT64_MAX) {
        free(p);
        free(pi);
        return fail();
      }
      pos++;
    }
  free(p);
  free(pi);
  return finish();
}
