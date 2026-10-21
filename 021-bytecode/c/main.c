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
static size_t width(unsigned op) {
  if (op == 1)
    return 5;
  if (op == 6 || op == 7)
    return 2;
  if (op == 8 || op == 9)
    return 3;
  if (op <= 9)
    return 1;
  fail();
  return 0;
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
  /* Duas passagens: tamanhos primeiro, alvos depois. Assim saltos para
     operandos são rejeitados mesmo em código que nunca será executado. */
  unsigned char boundary[65537] = {0};
  for (size_t p = 0; p < len;) {
    boundary[p] = 1;
    size_t n = width(data[p]);
    if (n > len - p)
      fail();
    if ((data[p] == 6 || data[p] == 7) && data[p + 1] >= 16)
      fail();
    p += n;
  }
  for (size_t p = 0; p < len; p += width(data[p]))
    if ((data[p] == 8 || data[p] == 9) &&
        (u16(p + 1) >= len || !boundary[u16(p + 1)]))
      fail();
  /* Invariantes: top <= 64; endereços já validados; next só aponta para
     uma fronteira ou fim de código (rejeitado na próxima busca). */
  int32_t stack[64], mem[16] = {0};
  size_t top = 0, pc = 0;
  /* HALT também consome uma instrução do orçamento. */
  for (unsigned steps = 0; steps < 1000; steps++) {
    if (pc >= len)
      fail();
    unsigned op = data[pc];
    size_t next = pc + width(op);
    if (op == 0) {
      if (top != 1)
        fail();
      printf("%ld\n", (long)stack[0]);
      output("", 0);
      finish();
      return 0;
    }
    if (op == 1) {
      if (top == 64)
        fail();
      uint32_t u = (uint32_t)data[pc + 1] | ((uint32_t)data[pc + 2] << 8) |
                   ((uint32_t)data[pc + 3] << 16) |
                   ((uint32_t)data[pc + 4] << 24);
      int64_t v = u;
      if (u > INT32_MAX)
        v -= INT64_C(4294967296);
      stack[top++] = (int32_t)v;
    } else if (op >= 2 && op <= 4) {
      if (top < 2)
        fail();
      int32_t b = stack[--top];
      stack[top - 1] = calc(stack[top - 1], b,
                            op == 2   ? '+'
                            : op == 3 ? '-'
                                      : '*');
    } else if (op == 5) {
      if (!top || top == 64)
        fail();
      int32_t v = stack[top - 1];
      stack[top++] = v;
    } else if (op == 6) {
      if (top == 64)
        fail();
      stack[top++] = mem[data[pc + 1]];
    } else if (op == 7) {
      if (!top)
        fail();
      mem[data[pc + 1]] = stack[--top];
    } else if (op == 8) {
      next = u16(pc + 1);
    } else if (op == 9) {
      if (!top)
        fail();
      if (stack[--top] == 0)
        next = u16(pc + 1);
    }
    pc = next;
  }
  fail();
  return 1;
}
