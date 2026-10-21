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
static const char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int value(int ch) {
  const char *p = ch > 0 ? strchr(alphabet, ch) : NULL;
  return p ? (int)(p - alphabet) : -1;
}
int main(int argc, char **argv) {
  setup();
  if (argc != 2)
    return fail();
  if (!strcmp(argv[1], "encode")) {
    unsigned char b[3];
    size_t n;
    while ((n = fread(b, 1, 3, stdin))) {
      putchar(alphabet[b[0] >> 2]);
      putchar(alphabet[((b[0] & 3) << 4) | (n > 1 ? b[1] >> 4 : 0)]);
      putchar(n > 1 ? alphabet[((b[1] & 15) << 2) | (n > 2 ? b[2] >> 6 : 0)]
                    : '=');
      putchar(n > 2 ? alphabet[b[2] & 63] : '=');
    }
  } else if (!strcmp(argv[1], "decode")) {
    int ch;
    while ((ch = getchar()) != EOF) {
      int q[4] = {ch, getchar(), getchar(), getchar()};
      int a = value(q[0]), b = value(q[1]);
      if (a < 0 || b < 0 || q[2] == EOF || q[3] == EOF)
        return fail();
      /* Padding so pode encerrar o fluxo; os bits descartados precisam ser
       * zero. */
      if (q[2] == '=') {
        if (q[3] != '=' || (b & 15) || getchar() != EOF)
          return fail();
        putchar((a << 2) | (b >> 4));
        break;
      }
      int c = value(q[2]);
      if (c < 0)
        return fail();
      if (q[3] == '=') {
        if ((c & 3) || getchar() != EOF)
          return fail();
        putchar((a << 2) | (b >> 4));
        putchar((b << 4) | (c >> 2));
        break;
      }
      int d = value(q[3]);
      if (d < 0)
        return fail();
      putchar((a << 2) | (b >> 4));
      putchar((b << 4) | (c >> 2));
      putchar((c << 6) | d);
    }
  } else
    return fail();
  return finish();
}
