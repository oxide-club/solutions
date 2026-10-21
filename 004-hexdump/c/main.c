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
int main(int argc, char **argv) {
  setup();
  if (argc > 2)
    return fail();
  FILE *f = stdin;
  if (argc == 2 && !(f = fopen(argv[1], "rb")))
    return fail();
  unsigned char b[16];
  size_t n;
  uint64_t offset = 0;
  while ((n = fread(b, 1, 16, f)) != 0) {
    printf("%08llx  ", (unsigned long long)offset);
    for (size_t i = 0; i < 16; i++) {
      if (i < n)
        printf("%02x ", b[i]);
      else
        fputs("   ", stdout);
    }
    fputs(" |", stdout);
    for (size_t i = 0; i < n; i++)
      putchar(b[i] >= 32 && b[i] <= 126 ? b[i] : '.');
    puts("|");
    if (offset > UINT64_MAX - n)
      return fail();
    offset += n;
  }
  int bad = ferror(f);
  if (f != stdin && fclose(f))
    bad = 1;
  if (bad)
    return fail();
  return finish();
}
