/* SPDX-License-Identifier: MIT */
#include "calc.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct test_case {
    int a, b, null_output, before;
};
static const struct test_case cases[] = {{7, 2, 0, 71}, {-7, 2, 0, 72},
                                         {7, 0, 0, 73}, {INT_MIN, -1, 0, 74},
                                         {7, 0, 1, 75}, {INT_MIN, -1, 1, 76}};
static const char *mode = "healthy";
static _Thread_local int reentered;
static pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;

int ox_trace_sink(const char *line, size_t length) {
    if (strcmp(mode, "reenter") == 0 && !reentered) {
        reentered = 1;
        int value;
        (void)calc_div(8, 2, &value);
    }
    if (strcmp(mode, "errno") == 0) {
        errno = EIO;
    }
    if (strcmp(mode, "fail") == 0) {
        return -1;
    }
    pthread_mutex_lock(&output_lock);
    int result = fwrite(line, 1, length, stderr) == length ? 0 : -1;
    pthread_mutex_unlock(&output_lock);
    return result;
}

static void *worker(void *unused) {
    (void)unused;
    char output[8192];
    size_t used = 0;
    for (unsigned i = 0; i < 60; i++) {
        const struct test_case *c = &cases[i % 6];
        int value = 123;
        errno = c->before;
        int rc = calc_div(c->a, c->b, c->null_output ? NULL : &value);
        int after = errno;
        /* Cada linha contém tudo o que a comparação funcional precisa. */
        int length = snprintf(output + used, sizeof output - used,
                              "case=%u rc=%d value=%d errno=%d\n", i % 6, rc, value, after);
        if (length < 0 || (size_t)length >= sizeof output - used) {
            return (void *)1;
        }
        used += (size_t)length;
    }
    /* Os quatro blocos iguais tornam stdout determinístico sem serializar chamadas. */
    flockfile(stdout);
    size_t written = fwrite(output, 1, used, stdout);
    funlockfile(stdout);
    if (written != used) {
        return (void *)1;
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc > 2 ||
        (argc == 2 && strcmp(argv[1], "healthy") != 0 && strcmp(argv[1], "errno") != 0 &&
         strcmp(argv[1], "fail") != 0 && strcmp(argv[1], "reenter") != 0)) {
        fprintf(stderr, "app: use [healthy|errno|fail|reenter]\n");
        return 2;
    }
    if (argc == 2) {
        mode = argv[1];
    }
    pthread_t threads[4];
    unsigned started = 0;
    for (; started < 4; started++) {
        if (pthread_create(&threads[started], NULL, worker, NULL) != 0) {
            break;
        }
    }
    int status = started == 4 ? 0 : 1;
    for (unsigned i = 0; i < started; i++) {
        void *result;
        if (pthread_join(threads[i], &result) != 0 || result != NULL) {
            status = 1;
        }
    }
    if (status != 0 || fflush(stdout) != 0) {
        fprintf(stderr, "app: falha de thread ou de escrita\n");
        return 1;
    }
    return 0;
}
