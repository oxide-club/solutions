/* SPDX-License-Identifier: MIT */
#define _GNU_SOURCE
#include "calc.h"
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

typedef int (*divide_fn)(int, int, int *);
typedef int (*sink_fn)(const char *, size_t);

static pthread_once_t once = PTHREAD_ONCE_INIT;
static divide_fn real_divide;
static sink_fn sink;
static atomic_uint_fast64_t next_id = 1;
static _Thread_local bool inside;

static void instrumentation_failure(void) {
    static const char message[] = "libtrace: não foi possível resolver uma função chamável\n";
    (void)write(STDERR_FILENO, message, sizeof message - 1);
    _Exit(125);
}

static void *resolve(void *scope, const char *name) {
    (void)dlerror();
    void *address = dlsym(scope, name);
    const char *error = dlerror();
    if (error != NULL || address == NULL) {
        instrumentation_failure();
    }
    return address;
}

static void initialize(void) {
    /* A ABI POSIX permite a conversão. memcpy evita cast rejeitado por -Wpedantic. */
    void *address = resolve(RTLD_NEXT, "calc_div");
    _Static_assert(sizeof address == sizeof real_divide, "ABI de ponteiro incompatível");
    memcpy(&real_divide, &address, sizeof real_divide);
    if (real_divide == calc_div) {
        instrumentation_failure();
    }
    address = resolve(RTLD_DEFAULT, "ox_trace_sink");
    _Static_assert(sizeof address == sizeof sink, "ABI de callback incompatível");
    memcpy(&sink, &address, sizeof sink);
}

/* A publicação via pthread_once precede toda chamada ao callback. */
int calc_div(int a, int b, int *out) {
    int before = errno;
    if (pthread_once(&once, initialize) != 0) {
        instrumentation_failure();
    }
    errno = before;
    int result = real_divide(a, b, out);
    int after = errno;
    if (!inside) {
        /* A guarda abrange formatação e callback; chamadas aninhadas só delegam. */
        inside = true;
        uint64_t id = atomic_fetch_add_explicit(&next_id, 1, memory_order_relaxed);
        if (id == 0 || id == UINT64_MAX) {
            instrumentation_failure();
        }
        char value[32];
        if (result == 0 && out != NULL) {
            (void)snprintf(value, sizeof value, "%d", *out);
        } else {
            memcpy(value, "null", 5);
        }
        char line[512];
        int length =
            snprintf(line, sizeof line,
                     "{\"call_id\":%" PRIu64 ",\"thread_id\":%ld,\"a\":%d,\"b\":%d,"
                     "\"return\":%d,\"errno_before\":%d,\"errno_after\":%d,\"value\":%s}\n",
                     id, syscall(SYS_gettid), a, b, result, before, after, value);
        if (length > 0 && (size_t)length < sizeof line) {
            /* O sink recebe a linha inteira e é responsável por serializar escritas. */
            (void)sink(line, (size_t)length);
        }
        inside = false;
    }
    errno = after;
    return result;
}
