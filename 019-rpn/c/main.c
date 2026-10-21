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
/* int64 comporta soma, subtração e produto de quaisquer dois int32. */
static int32_t calc(int32_t a, int32_t b, int op) {
  int64_t v = op == '+'   ? (int64_t)a + b
              : op == '-' ? (int64_t)a - b
                          : (int64_t)a * b;
  if (v < INT32_MIN || v > INT32_MAX)
    fail();
  return (int32_t)v;
}
int main(int argc, char **argv) {
#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif
  (void)argv;
  if (argc != 1)
    fail();
  readall();
  int32_t stack[64];
  size_t top = 0, p = 0;
  while (p < len) {
    if (isspace(data[p])) {
      p++;
      continue;
    }
    size_t start = p;
    while (p < len && !isspace(data[p]))
      p++;
    size_t n = p - start;
    if (n == 1 &&
        (data[start] == '+' || data[start] == '-' || data[start] == '*')) {
      if (top < 2)
        fail();
      int32_t b = stack[--top];
      stack[top - 1] = calc(stack[top - 1], b, data[start]);
    } else {
      size_t j = start;
      int neg = data[j] == '-';
      if (neg)
        j++;
      if (j == p)
        fail();
      int64_t v = 0;
      for (; j < p; j++) {
        if (data[j] < '0' || data[j] > '9')
          fail();
        v = v * 10 + data[j] - '0';
        if (v > (int64_t)INT32_MAX + neg)
          fail();
      }
      if (top == 64)
        fail();
      stack[top++] = (int32_t)(neg ? -v : v);
    }
  }
  if (top != 1)
    fail();
  printf("%ld\n", (long)stack[0]);
  output("", 0);
  finish();
  return 0;
}
