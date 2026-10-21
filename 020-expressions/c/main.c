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
static size_t pos;
static unsigned depth;
static void space(void) {
  while (pos < len && isspace(data[pos]))
    pos++;
}
static int32_t expr(void);
static int32_t atom(void) {
  space();
  if (pos == len)
    fail();
  if (data[pos] == '(') {
    pos++;
    if (++depth > 64)
      fail();
    int32_t v = expr();
    space();
    if (pos == len || data[pos++] != ')')
      fail();
    depth--;
    return v;
  }
  if (data[pos] < '0' || data[pos] > '9')
    fail();
  int64_t v = 0;
  while (pos < len && data[pos] >= '0' && data[pos] <= '9') {
    v = v * 10 + data[pos++] - '0';
    if (v > INT32_MAX)
      fail();
  }
  return (int32_t)v;
}
static int32_t term(void) {
  int32_t v = atom();
  space();
  while (pos < len && data[pos] == '*') {
    pos++;
    v = calc(v, atom(), '*');
    space();
  }
  return v;
}
static int32_t expr(void) {
  int32_t v = term();
  space();
  while (pos < len && (data[pos] == '+' || data[pos] == '-')) {
    int op = data[pos++];
    v = calc(v, term(), op);
    space();
  }
  return v;
}
int main(int argc, char **argv) {
#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif
  (void)argv;
  if (argc != 1)
    fail();
  readall();
  int32_t v = expr();
  space();
  if (pos != len)
    fail();
  printf("%ld\n", (long)v);
  output("", 0);
  finish();
  return 0;
}
