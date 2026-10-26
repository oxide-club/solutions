/* SPDX-License-Identifier: MIT */
#define _GNU_SOURCE
#include "common.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

struct worker {
    struct ox_region *region;
    atomic_bool stop, invalid;
};

static void *update(void *argument) {
    struct worker *worker = argument;
    const struct timespec pause = {0, 10000000};
    while (!atomic_load(&worker->stop)) {
        nanosleep(&pause, NULL);
        if (atomic_load(&worker->stop)) {
            break;
        }
        struct timespec deadline = ox_deadline();
        int result = ox_lock(worker->region, &deadline);
        if (result != 0) {
            atomic_store(&worker->invalid, 1);
            break;
        }
        if (!ox_valid(worker->region) || worker->region->generation == UINT64_MAX) {
            atomic_store(&worker->invalid, 1);
        } else {
            worker->region->counter = (worker->region->counter + 1) % (OX_LIMIT + 1);
            worker->region->counter_xor = worker->region->counter ^ UINT64_MAX;
            ++worker->region->generation;
        }
        if (pthread_mutex_unlock(&worker->region->mutex) != 0) {
            atomic_store(&worker->invalid, 1);
        }
        if (atomic_load(&worker->invalid)) {
            break;
        }
    }
    return NULL;
}

static int error_reply(int fd, uint64_t sequence, const char *code,
                       const struct timespec *deadline) {
    char reply[96];
    snprintf(reply, sizeof(reply), "ERR %" PRIu64 " %s\n", sequence, code);
    return ox_send(fd, reply, -1, deadline);
}

static void read_error(int fd, int result) {
    /* O prazo de leitura acabou; a tentativa de diagnóstico tem prazo próprio. */
    struct timespec deadline = ox_deadline();
    const char *code = result == -2 ? "TIMEOUT" : result == -3 ? "FRAME" : "RESOURCE";
    (void)error_reply(fd, 0, code, &deadline);
}

static int authenticate(int fd, const char token[65], int memory) {
    struct timespec deadline = ox_deadline();
    struct ucred peer;
    socklen_t length = sizeof(peer);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &length) != 0 || length != sizeof(peer) ||
        peer.uid != getuid()) {
        (void)error_reply(fd, 0, "AUTH", &deadline);
        return 0;
    }
    char line[OX_LINE + 1];
    int result = ox_line(fd, line, &deadline);
    if (result != 0) {
        read_error(fd, result);
        return result == -3 ? 0 : -1;
    }
    char *fields[4] = {0};
    int count = ox_fields(line, fields, 4);
    const char *code = NULL;
    if (count != 3 || strcmp(fields[0], "HELLO") != 0) {
        code = "FRAME";
    } else if (strcmp(fields[1], "1") != 0) {
        code = "VERSION";
    } else if (strcmp(fields[2], token) != 0) {
        code = "AUTH";
    }
    if (code != NULL) {
        (void)error_reply(fd, 0, code, &deadline);
        return 0;
    }
    char reply[80];
    snprintf(reply, sizeof(reply), "OK 0 HELLO %zu\n", sizeof(struct ox_region));
    /* Uma falha depois da autenticação invalida a sessão. */
    return ox_send(fd, reply, memory, &deadline) == 0 ? 1 : -1;
}

static void session(int fd, struct ox_region *region, struct worker *worker, pthread_t *thread,
                    int *thread_started, uint64_t expected) {
    for (;;) {
        struct timespec deadline = ox_deadline();
        char line[OX_LINE + 1];
        int result = ox_line(fd, line, &deadline);
        if (result != 0) {
            read_error(fd, result);
            return;
        }
        char *fields[5] = {0};
        int count = ox_fields(line, fields, 5);
        uint64_t sequence = 0;
        int identifiable = fields[1] != NULL && ox_uint(fields[1], &sequence) == 0;
        if (count < 2 || !identifiable) {
            (void)error_reply(fd, identifiable ? sequence : 0, "FRAME", &deadline);
            return;
        }
        int snapshot = strcmp(fields[0], "SNAPSHOT") == 0;
        int set = strcmp(fields[0], "SET_COUNTER") == 0;
        int stop = strcmp(fields[0], "STOP") == 0;
        if (count != (set ? 3 : 2)) {
            (void)error_reply(fd, sequence, "FRAME", &deadline);
            return;
        }
        if (sequence != expected) {
            (void)error_reply(fd, sequence, "SEQ", &deadline);
            return;
        }
        if (atomic_load(&worker->invalid) || (expected == UINT64_MAX && !stop)) {
            (void)error_reply(fd, sequence, "STATE", &deadline);
            return;
        }
        if (stop) {
            atomic_store(&worker->stop, 1);
            if (thread != NULL) {
                if (pthread_join(*thread, NULL) != 0) {
                    (void)error_reply(fd, sequence, "RESOURCE", &deadline);
                    return;
                }
                *thread_started = 0;
            }
            if (atomic_load(&worker->invalid)) {
                (void)error_reply(fd, sequence, "STATE", &deadline);
                return;
            }
            char reply[80];
            snprintf(reply, sizeof(reply), "OK %" PRIu64 " STOP\n", sequence);
            (void)ox_send(fd, reply, -1, &deadline);
            return;
        }
        ++expected; /* UINT64_MAX já foi tratado; não ocorre wrap. */
        if (!snapshot && !set) {
            if (error_reply(fd, sequence, "COMMAND", &deadline) != 0) {
                return;
            }
            continue;
        }
        uint64_t value = 0;
        if (set && (ox_uint(fields[2], &value) != 0 || value > OX_LIMIT)) {
            if (error_reply(fd, sequence, "VALUE", &deadline) != 0) {
                return;
            }
            continue;
        }
        result = ox_lock(region, &deadline);
        if (result != 0) {
            struct timespec diagnostic = ox_deadline();
            (void)error_reply(fd, sequence, result == ETIMEDOUT ? "TIMEOUT" : "STATE", &diagnostic);
            return;
        }
        if (!ox_valid(region) || (set && region->generation == UINT64_MAX)) {
            pthread_mutex_unlock(&region->mutex);
            (void)error_reply(fd, sequence, "STATE", &deadline);
            return;
        }
        if (set) {
            /* A checagem anterior impede alteração parcial no overflow. */
            region->counter = value;
            region->counter_xor = value ^ UINT64_MAX;
            ++region->generation;
        }
        struct ox_state state = ox_copy(region);
        if (pthread_mutex_unlock(&region->mutex) != 0) {
            (void)error_reply(fd, sequence, "STATE", &deadline);
            return;
        }
        char reply[OX_LINE + 1];
        snprintf(reply, sizeof(reply), "OK %" PRIu64 " STATE %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
                 sequence, state.generation, state.counter, state.counter_xor);
        if (ox_send(fd, reply, -1, &deadline) != 0) {
            return;
        }
    }
}

static int random_token(unsigned char token[32]) {
    size_t used = 0;
    while (used < 32) {
        ssize_t count = getrandom(token + used, 32 - used, 0);
        if (count > 0) {
            used += (size_t)count;
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }
    return 0;
}

static int initialize(struct ox_region *region) {
    pthread_mutexattr_t attributes;
    int result = pthread_mutexattr_init(&attributes);
    if (result != 0) {
        return -1;
    }
    result = pthread_mutexattr_setpshared(&attributes, PTHREAD_PROCESS_SHARED);
    if (result == 0) {
        result = pthread_mutexattr_setrobust(&attributes, PTHREAD_MUTEX_ROBUST);
    }
    if (result == 0) {
        result = pthread_mutex_init(&region->mutex, &attributes);
    }
    int destroyed = pthread_mutexattr_destroy(&attributes);
    if (result != 0 || destroyed != 0) {
        return -1;
    }
    /* Ainda não há outro usuário: a publicação ocorre somente após HELLO. */
    region->version = 1;
    region->generation = 0;
    region->counter = 0;
    region->counter_xor = UINT64_MAX;
    return 0;
}

static int reap(pid_t child) {
    struct timespec deadline = ox_deadline();
    for (;;) {
        int status;
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
        }
        if (result < 0 && errno != EINTR) {
            return -1;
        }
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec > deadline.tv_sec ||
            (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec)) {
            kill(child, SIGKILL);
            while (waitpid(child, NULL, 0) < 0 && errno == EINTR) {
            }
            return -1;
        }
        const struct timespec pause = {0, 10000000};
        nanosleep(&pause, NULL);
    }
}

int main(int argc, char **argv) {
    const char *observer = NULL;
    int with_worker = 0;
    uint64_t expected = 1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--observer") == 0 && i + 1 < argc && observer == NULL) {
            observer = argv[++i];
        } else if (strcmp(argv[i], "--test-worker") == 0 && !with_worker) {
            with_worker = 1;
#ifdef OX_TEST_HOOKS
        } else if (strcmp(argv[i], "--test-sequence-max") == 0 && expected == 1) {
            expected = UINT64_MAX;
#endif
        } else {
            fprintf(stderr, "target: argumento inválido\n");
            return 2;
        }
    }
    if (observer == NULL || observer[0] != '/') {
        fprintf(stderr, "target: use --observer /caminho/absoluto/observer\n");
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    int status = 1, listener = -1, peer = -1, memory = -1;
    int token_pipe[2] = {-1, -1};
    int initialized = 0, directory_created = 0, socket_created = 0;
    int thread_started = 0;
    pid_t child = -1;
    pthread_t thread;
    struct ox_region *region = MAP_FAILED;
    struct worker worker = {.region = NULL, .stop = 0, .invalid = 0};
    char directory[sizeof(((struct sockaddr_un *)0)->sun_path)];
    struct sockaddr_un address = {.sun_family = AF_UNIX};
    const char *temporary = getenv("TMPDIR");
    if (temporary == NULL) {
        temporary = "/tmp";
    }
    int length = snprintf(directory, sizeof(directory), "%s/oxide-ipc-XXXXXX", temporary);
    if (length < 0 || (size_t)length >= sizeof(directory) - 2 || mkdtemp(directory) == NULL) {
        goto cleanup;
    }
    directory_created = 1;
    if (chmod(directory, 0700) != 0) {
        goto cleanup;
    }
    memcpy(address.sun_path, directory, (size_t)length);
    memcpy(address.sun_path + length, "/s", 3);
    listener = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (listener < 0 || bind(listener, (struct sockaddr *)&address, sizeof(address)) != 0) {
        goto cleanup;
    }
    socket_created = 1;
    if (chmod(address.sun_path, 0600) != 0 || listen(listener, 4) != 0) {
        goto cleanup;
    }
    memory = memfd_create("oxide-snapshot", MFD_CLOEXEC);
    if (memory < 0 || ftruncate(memory, sizeof(*region)) != 0) {
        goto cleanup;
    }
    region = mmap(NULL, sizeof(*region), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0);
    if (region == MAP_FAILED || initialize(region) != 0) {
        goto cleanup;
    }
    initialized = 1;
    unsigned char token[32];
    char hexadecimal[65];
    if (random_token(token) != 0 || pipe2(token_pipe, O_CLOEXEC | O_NONBLOCK) != 0) {
        goto cleanup;
    }
    for (size_t i = 0; i < 32; ++i) {
        snprintf(hexadecimal + 2 * i, 3, "%02x", token[i]);
    }
    child = fork();
    if (child < 0) {
        goto cleanup;
    }
    if (child == 0) {
        close(token_pipe[1]);
        /* Só o pipe de token atravessa exec; o segredo não está em argv. */
        if (fcntl(token_pipe[0], F_SETFD, 0) != 0 || fcntl(token_pipe[0], F_SETFL, 0) != 0) {
            _exit(1);
        }
        char descriptor[32];
        snprintf(descriptor, sizeof(descriptor), "%d", token_pipe[0]);
        char *arguments[] = {(char *)observer, address.sun_path, descriptor, NULL};
        execv(observer, arguments);
        _exit(1);
    }
    close(token_pipe[0]);
    token_pipe[0] = -1;
    struct timespec deadline = ox_deadline();
    if (ox_write(token_pipe[1], token, sizeof(token), &deadline) != 0) {
        goto cleanup;
    }
    memset(token, 0, sizeof(token));
    close(token_pipe[1]);
    token_pipe[1] = -1;
    for (;;) {
        deadline = ox_deadline();
        if (ox_wait(listener, POLLIN, &deadline) != 0) {
            goto cleanup;
        }
        peer = accept4(listener, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK);
        if (peer < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            goto cleanup;
        }
        int authenticated = authenticate(peer, hexadecimal, memory);
        if (authenticated == 0) {
            close(peer);
            peer = -1;
            continue;
        }
        if (authenticated < 0) {
            break;
        }
        worker.region = region;
        if (with_worker) {
            if (pthread_create(&thread, NULL, update, &worker) != 0) {
                deadline = ox_deadline();
                (void)error_reply(peer, 0, "RESOURCE", &deadline);
                goto cleanup;
            }
            thread_started = 1;
        }
        session(peer, region, &worker, thread_started ? &thread : NULL, &thread_started, expected);
        break;
    }
    status = 0;

cleanup:
    atomic_store(&worker.stop, 1);
    /* STOP faz join antes da resposta; falhas também precisam encerrar a worker. */
    if (thread_started) {
        if (pthread_join(thread, NULL) != 0) {
            status = 1;
        }
    }
    if (peer >= 0 && close(peer) != 0) {
        status = 1;
    }
    if (token_pipe[0] >= 0 && close(token_pipe[0]) != 0) {
        status = 1;
    }
    if (token_pipe[1] >= 0 && close(token_pipe[1]) != 0) {
        status = 1;
    }
    if (child > 0 && reap(child) != 0) {
        status = 1;
    }
    /* Todos os usuários cooperativos terminaram antes de destruir/desmapear. */
    if (initialized) {
        deadline = ox_deadline();
        int result = ox_lock(region, &deadline);
        if (result == 0) {
            if (pthread_mutex_unlock(&region->mutex) != 0 ||
                pthread_mutex_destroy(&region->mutex) != 0) {
                status = 1;
            }
        } else if (result != EOWNERDEAD && result != ENOTRECOVERABLE) {
            status = 1;
        }
    }
    if (region != MAP_FAILED && munmap(region, sizeof(*region)) != 0) {
        status = 1;
    }
    if (memory >= 0 && close(memory) != 0) {
        status = 1;
    }
    if (listener >= 0 && close(listener) != 0) {
        status = 1;
    }
    if (socket_created && unlink(address.sun_path) != 0) {
        status = 1;
    }
    if (directory_created && rmdir(directory) != 0) {
        status = 1;
    }
    if (status != 0) {
        fprintf(stderr, "target: falha de sessão, processo filho ou liberação de recursos\n");
    }
    return status;
}
