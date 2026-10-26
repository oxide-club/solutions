/* SPDX-License-Identifier: MIT */
#define _GNU_SOURCE
#include "common.h"
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static int token_read(int fd, char hexadecimal[65]) {
    unsigned char token[32];
    size_t used = 0;
    struct timespec deadline = ox_deadline();
    while (used < sizeof(token)) {
        if (ox_wait(fd, POLLIN, &deadline) != 0) {
            return -1;
        }
        ssize_t count = read(fd, token + used, sizeof(token) - used);
        if (count > 0) {
            used += (size_t)count;
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }
    for (size_t i = 0; i < sizeof(token); ++i) {
        snprintf(hexadecimal + 2 * i, 3, "%02x", token[i]);
    }
    memset(token, 0, sizeof(token));
    return 0;
}

static int connect_peer(int fd, const struct sockaddr_un *address,
                        const struct timespec *deadline) {
    if (connect(fd, (const struct sockaddr *)address, sizeof(*address)) == 0) {
        return 0;
    }
    if (errno != EINPROGRESS && errno != EAGAIN) {
        return -1;
    }
    if (ox_wait(fd, POLLOUT, deadline) != 0) {
        return -1;
    }
    int error;
    socklen_t size = sizeof(error);
    return getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size) == 0 && error == 0 ? 0 : -1;
}

static int state_response(int fd, struct ox_region *region, uint64_t sequence, int set,
                          struct ox_state *previous) {
    struct timespec deadline = ox_deadline();
    char line[OX_LINE + 1];
    if (set) {
        snprintf(line, sizeof(line), "SET_COUNTER %" PRIu64 " 7\n", sequence);
    } else {
        snprintf(line, sizeof(line), "SNAPSHOT %" PRIu64 "\n", sequence);
    }
    if (ox_send(fd, line, -1, &deadline) != 0 || ox_line(fd, line, &deadline) != 0) {
        return -1;
    }
    char *fields[7] = {0};
    uint64_t received;
    struct ox_state reply;
    int count = ox_fields(line, fields, 7);
    if (count != 6 || strcmp(fields[0], "OK") != 0 || strcmp(fields[2], "STATE") != 0 ||
        ox_uint(fields[1], &received) != 0 || received != sequence ||
        ox_uint(fields[3], &reply.generation) != 0 || ox_uint(fields[4], &reply.counter) != 0 ||
        ox_uint(fields[5], &reply.counter_xor) != 0 || reply.counter > OX_LIMIT ||
        reply.counter_xor != (reply.counter ^ UINT64_MAX) ||
        reply.generation < previous->generation ||
        (reply.generation == previous->generation &&
         (reply.counter != previous->counter || reply.counter_xor != previous->counter_xor)) ||
        (set && (reply.counter != 7 || reply.generation <= previous->generation))) {
        return -1;
    }
    if (ox_lock(region, &deadline) != 0) {
        return -1;
    }
    /* Nem versão nem redundância são consultadas fora da exclusão mútua. */
    int valid = ox_valid(region);
    struct ox_state later = {0};
    if (valid) {
        later = ox_copy(region);
    }
    if (pthread_mutex_unlock(&region->mutex) != 0) {
        return -1;
    }
    if (!valid || later.generation < reply.generation ||
        (later.generation == reply.generation &&
         (later.counter != reply.counter || later.counter_xor != reply.counter_xor))) {
        return -1;
    }
    printf("{\"seq\":%" PRIu64 ",\"response_generation\":%" PRIu64 ",\"later_generation\":%" PRIu64
           ",\"counter\":%" PRIu64 "}\n",
           sequence, reply.generation, later.generation, reply.counter);
    *previous = reply;
    return 0;
}

int main(int argc, char **argv) {
    uint64_t pipe_number;
    if (argc != 3 || ox_uint(argv[2], &pipe_number) != 0 || pipe_number > INT_MAX ||
        strlen(argv[1]) >= sizeof(((struct sockaddr_un *)0)->sun_path)) {
        fprintf(stderr, "observer: use <socket-path> <token-fd>\n");
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    int status = 1, fd = -1, memory = -1;
    struct ox_region *region = MAP_FAILED;
    char token[65], line[OX_LINE + 1];
    int token_ok = token_read((int)pipe_number, token);
    int token_closed = close((int)pipe_number);
    if (token_ok != 0 || token_closed != 0) {
        goto cleanup;
    }
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    struct sockaddr_un address = {.sun_family = AF_UNIX};
    strcpy(address.sun_path, argv[1]);
    struct timespec deadline = ox_deadline();
    if (fd < 0 || connect_peer(fd, &address, &deadline) != 0) {
        goto cleanup;
    }
    snprintf(line, sizeof(line), "HELLO 1 %s\n", token);
    memset(token, 0, sizeof(token));
    if (ox_send(fd, line, -1, &deadline) != 0 || ox_hello(fd, line, &memory, &deadline) != 0) {
        goto cleanup;
    }
    char expected[80];
    snprintf(expected, sizeof(expected), "OK 0 HELLO %zu", sizeof(*region));
    struct stat metadata;
    if (strcmp(line, expected) != 0 || fstat(memory, &metadata) != 0 ||
        metadata.st_size != (off_t)sizeof(*region)) {
        goto cleanup;
    }
    region = mmap(NULL, sizeof(*region), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0);
    if (region == MAP_FAILED || ox_lock(region, &deadline) != 0) {
        goto cleanup;
    }
    int valid = ox_valid(region);
    struct ox_state previous = {0};
    if (valid) {
        previous = ox_copy(region);
    }
    int unlocked = pthread_mutex_unlock(&region->mutex);
    if (!valid || unlocked != 0) {
        goto cleanup;
    }
    /* A atribuição repetida também precisa avançar a geração. */
    for (uint64_t sequence = 1; sequence <= 5; ++sequence) {
        if (state_response(fd, region, sequence, sequence == 2 || sequence == 4, &previous) != 0) {
            goto cleanup;
        }
        if (sequence == 2 || sequence == 4) {
            /* Abra um intervalo para observar a worker, sem supor que ela rode. */
            const struct timespec pause = {0, 20000000};
            nanosleep(&pause, NULL);
        }
    }
    deadline = ox_deadline();
    if (ox_send(fd, "STOP 6\n", -1, &deadline) != 0 || ox_line(fd, line, &deadline) != 0 ||
        strcmp(line, "OK 6 STOP") != 0) {
        goto cleanup;
    }
    status = fflush(stdout) == 0 ? 0 : 1;

cleanup:
    if (region != MAP_FAILED && munmap(region, sizeof(*region)) != 0) {
        status = 1;
    }
    if (memory >= 0 && close(memory) != 0) {
        status = 1;
    }
    if (fd >= 0 && close(fd) != 0) {
        status = 1;
    }
    if (status != 0) {
        fprintf(stderr, "observer: sessão inválida ou falha ao liberar recursos\n");
    }
    return status;
}
