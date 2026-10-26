/* SPDX-License-Identifier: MIT */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum state { NEW, STARTING, STOPPED, RUNNING, EXITED, FAILED };
struct session {
    enum state state;
    pid_t child;
    bool live;
    unsigned long sequence;
    int pending_signal;
    int result;
};
struct child_error {
    int operation; /* 1: TRACEME; 2: exec. */
    int number;
};
static bool output_failed;
static const char *const names[] = {"NEW", "STARTING", "STOPPED", "RUNNING", "EXITED", "FAILED"};

static void event_begin(struct session *s, const char *cause) {
    printf("{\"seq\":%lu,\"state\":\"%s\",\"cause\":\"%s\",\"pid\":%ld", ++s->sequence,
           names[s->state], cause, (long)s->child);
}
static void event_end(void) {
    puts("}");
    /* Clientes interativos aguardam cada evento. Falha de saída encerra a sessão. */
    if (fflush(stdout) == EOF) {
        output_failed = true;
    }
}
static void event(struct session *s, const char *cause) {
    event_begin(s, cause);
    event_end();
}
static int fail(struct session *s, const char *cause, int number) {
    s->state = FAILED;
    s->result = 1;
    event_begin(s, cause);
    printf(",\"errno\":%d", number);
    event_end();
    fprintf(stderr, "oxdbg: %s: %s\n", cause, strerror(number));
    return -1;
}

/* Cada espera compartilha um único prazo; EINTR nunca renova esse prazo. */
static int wait_event(struct session *s, int *status) {
    struct timespec start, now;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        return fail(s, "clock_error", errno);
    }
    for (;;) {
        pid_t got = waitpid(s->child, status, WNOHANG);
        if (got == s->child) {
            return 0;
        }
        if (got < 0 && errno != EINTR) {
            return fail(s, "wait_error", errno);
        }
        if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
            return fail(s, "clock_error", errno);
        }
        int64_t elapsed =
            (int64_t)(now.tv_sec - start.tv_sec) * 1000000000 + now.tv_nsec - start.tv_nsec;
        if (elapsed >= INT64_C(5000000000)) {
            s->state = FAILED;
            s->result = 1;
            event(s, "timeout");
            fprintf(stderr, "oxdbg: prazo de cinco segundos excedido\n");
            return -1;
        }
        struct timespec pause = {.tv_nsec = 1000000};
        nanosleep(&pause, NULL);
    }
}

static void log_stop(struct session *s, const char *cause, int status, const siginfo_t *info) {
    event_begin(s, cause);
    printf(",\"raw_status\":%d,\"signal\":%d,\"event\":%u,\"siginfo\":", status, WSTOPSIG(status),
           (unsigned int)status >> 16);
    if (info) {
        printf("{\"code\":%d,\"signo\":%d}", info->si_code, info->si_signo);
    } else {
        fputs("null", stdout);
    }
    event_end();
}
static void log_exit(struct session *s, int status) {
    s->live = false; /* waitpid já recolheu o filho. */
    s->state = EXITED;
    event_begin(s, "exit");
    if (WIFEXITED(status)) {
        printf(",\"exit_code\":%d", WEXITSTATUS(status));
    } else {
        printf(",\"term_signal\":%d", WTERMSIG(status));
    }
    event_end();
}

/* Não se usa SIGTRAP isoladamente para reconhecer single-step. */
static int observe(struct session *s, int status, bool stepping, int injected) {
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        log_exit(s, status);
        return 0;
    }
    if (!WIFSTOPPED(status)) {
        return fail(s, "status_error", EPROTO);
    }
    siginfo_t info;
    bool known = ptrace(PTRACE_GETSIGINFO, s->child, NULL, &info) == 0;
    unsigned int code = (unsigned int)status >> 16;
    int sig = WSTOPSIG(status);
    s->state = STOPPED;
    s->pending_signal = 0;
    const char *cause;
    if (code == PTRACE_EVENT_EXEC && sig == SIGTRAP) {
        cause = "exec";
    } else if (code != 0 || !known) {
        cause = "unsupported_stop";
    } else if (stepping && sig == SIGTRAP && info.si_code == TRAP_TRACE) {
        cause = "step";
    } else if (stepping && injected && sig == SIGTRAP && info.si_code == SIGTRAP) {
        cause = "signal_step";
    } else if (info.si_code <= 0 || info.si_code == SI_KERNEL ||
               (sig == SIGTRAP && info.si_code == TRAP_BRKPT)) {
        cause = "signal_delivery";
        s->pending_signal = sig;
    } else {
        cause = "unsupported_stop";
    }
    if (!strcmp(cause, "unsupported_stop")) {
        s->state = FAILED;
        s->result = 1;
    }
    log_stop(s, cause, status, known ? &info : NULL);
    return s->state == FAILED ? -1 : 0;
}
static int resume(struct session *s, bool stepping) {
    int injected = s->pending_signal;
    if (ptrace(stepping ? PTRACE_SINGLESTEP : PTRACE_CONT, s->child, NULL,
               (void *)(intptr_t)injected) < 0) {
        return fail(s, "resume_error", errno);
    }
    s->pending_signal = 0; /* O sinal pertence apenas a esta retomada. */
    s->state = RUNNING;
    int status;
    if (wait_event(s, &status) < 0) {
        return -1;
    }
    return observe(s, status, stepping, injected);
}

static void report_child_error(int fd, int operation) {
    struct child_error error = {operation, errno};
    const char *bytes = (const char *)&error;
    size_t left = sizeof(error);
    while (left) {
        ssize_t sent = write(fd, bytes, left);
        if (sent < 0 && errno == EINTR) {
            continue;
        }
        if (sent <= 0) {
            break;
        }
        bytes += sent;
        left -= (size_t)sent;
    }
    _exit(1);
}
static int start(struct session *s, char **target) {
    int channel[2];
    if (pipe2(channel, O_CLOEXEC | O_NONBLOCK) < 0) {
        return fail(s, "pipe_error", errno);
    }
    s->state = STARTING;
    s->child = fork();
    if (s->child < 0) {
        int number = errno;
        s->child = 0;
        close(channel[0]);
        close(channel[1]);
        return fail(s, "fork_error", number);
    }
    if (!s->child) {
        close(channel[0]);
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
            report_child_error(channel[1], 1);
        }
        raise(SIGSTOP);
        execvp(target[0], target);
        report_child_error(channel[1], 2);
    }
    s->live = true;
    close(channel[1]);
    int status;
    if (wait_event(s, &status) < 0) {
        close(channel[0]);
        return -1;
    }
    struct child_error error;
    ssize_t got = read(channel[0], &error, sizeof(error));
    if (got == sizeof(error)) {
        s->live = false; /* A tentativa TRACEME falhou antes da parada inicial. */
        s->state = FAILED;
        if (error.operation == 1 &&
            (error.number == EPERM || error.number == EACCES || error.number == ENOSYS)) {
            s->result = 77;
            event_begin(s, "unavailable");
            printf(
                ",\"operation\":\"PTRACE_TRACEME\",\"errno\":%d,"
                "\"evidence\":{\"kind\":\"child_ptrace_result\",\"child_pid\":%ld,\"result\":-1}",
                error.number, (long)s->child);
            event_end();
            fprintf(stderr, "oxdbg: PTRACE_TRACEME no filho %ld retornou -1, errno=%d: %s\n",
                    (long)s->child, error.number, strerror(error.number));
        } else {
            fail(s, "child_error", error.number);
        }
        close(channel[0]);
        return -1;
    }
    if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGSTOP) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            s->live = false;
        }
        close(channel[0]);
        return fail(s, "sync_error", EPROTO);
    }
    siginfo_t sync_info;
    bool sync_known = ptrace(PTRACE_GETSIGINFO, s->child, NULL, &sync_info) == 0;
    log_stop(s, "sync", status, sync_known ? &sync_info : NULL);
    long options = PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL | PTRACE_O_TRACESYSGOOD;
    if (ptrace(PTRACE_SETOPTIONS, s->child, NULL, (void *)options) < 0 ||
        ptrace(PTRACE_CONT, s->child, NULL, NULL) < 0) {
        int number = errno;
        close(channel[0]);
        return fail(s, "setup_error", number);
    }
    s->state = RUNNING;
    if (wait_event(s, &status) < 0) {
        close(channel[0]);
        return -1;
    }
    got = read(channel[0], &error, sizeof(error));
    close(channel[0]);
    if (got == sizeof(error)) {
        s->live = false;
        return fail(s, "exec_error", error.number);
    }
    if (observe(s, status, false, 0) < 0) {
        return -1;
    }
    if (s->state != STOPPED || ((unsigned int)status >> 16) != PTRACE_EVENT_EXEC) {
        return fail(s, "exec_error", EPROTO);
    }
    /* O evento exec precede a saída de execve. Drene-a antes de contar instruções. */
    if (ptrace(PTRACE_SYSCALL, s->child, NULL, NULL) < 0) {
        return fail(s, "exec_boundary_error", errno);
    }
    s->state = RUNNING;
    if (wait_event(s, &status) < 0) {
        return -1;
    }
    if (!WIFSTOPPED(status) || WSTOPSIG(status) != (SIGTRAP | 0x80) ||
        ((unsigned int)status >> 16) != 0) {
        observe(s, status, false, 0);
        return fail(s, "exec_boundary_error", EPROTO);
    }
    s->state = STOPPED;
    siginfo_t info;
    bool known = ptrace(PTRACE_GETSIGINFO, s->child, NULL, &info) == 0;
    log_stop(s, "exec_boundary", status, known ? &info : NULL);
    return 0;
}

static int registers(struct session *s) {
    struct user_regs_struct r;
    if (ptrace(PTRACE_GETREGS, s->child, NULL, &r) < 0) {
        return fail(s, "regs_error", errno);
    }
    event_begin(s, "regs");
    printf(",\"regs\":{\"RIP\":\"0x%llx\",\"RSP\":\"0x%llx\","
           "\"RAX\":\"0x%llx\",\"RBX\":\"0x%llx\",\"RCX\":\"0x%llx\","
           "\"RDX\":\"0x%llx\",\"RFLAGS\":\"0x%llx\"}",
           r.rip, r.rsp, r.rax, r.rbx, r.rcx, r.rdx, r.eflags);
    event_end();
    return 0;
}
static void cleanup(struct session *s) {
    if (!s->live) {
        return;
    }
    /* SIGKILL também termina tracees parados; nenhum detach deixa o filho solto. */
    if (kill(s->child, SIGKILL) < 0 && errno != ESRCH) {
        fail(s, "kill_error", errno);
    }
    int status;
    for (;;) {
        pid_t got = waitpid(s->child, &status, 0);
        if (got < 0 && errno == EINTR) {
            continue;
        }
        if (got < 0) {
            fail(s, "reap_error", errno);
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            break;
        }
        /* Uma parada já pendente pode ser observada antes do SIGKILL. */
        if (ptrace(PTRACE_CONT, s->child, NULL, (void *)(intptr_t)SIGKILL) < 0 && errno != ESRCH) {
            fail(s, "cleanup_error", errno);
        }
    }
    s->live = false;
}
int main(int argc, char **argv) {
    if (argc < 3 || strcmp(argv[1], "--")) {
        fprintf(stderr, "oxdbg: uso: oxdbg -- caminho-do-alvo [argumentos]\n");
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    struct session s = {.state = NEW};
    event(&s, "new");
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    while (!output_failed && (length = getline(&line, &capacity, stdin)) >= 0) {
        /* Exige uma linha ASCII completa, sem truncar comandos longos. */
        if (length == 0 || line[length - 1] != '\n' || memchr(line, '\0', (size_t)length)) {
            event(&s, "rejected");
            continue;
        }
        line[length - 1] = '\0';
        if (!strcmp(line, "quit")) {
            event(&s, "quit");
            break;
        }
        if (!strcmp(line, "run") && s.state == NEW) {
            if (start(&s, argv + 2) < 0) {
                break;
            }
        } else if (!strcmp(line, "regs") && s.state == STOPPED) {
            if (registers(&s) < 0) {
                break;
            }
        } else if ((!strcmp(line, "step") || !strcmp(line, "continue")) && s.state == STOPPED) {
            if (resume(&s, !strcmp(line, "step")) < 0) {
                break;
            }
        } else {
            event(&s, "rejected");
        }
    }
    if (feof(stdin)) {
        event(&s, "eof");
    } else if (ferror(stdin)) {
        fail(&s, "input_error", errno ? errno : EIO);
    }
    free(line);
    cleanup(&s);
    if (output_failed || fflush(stdout) == EOF) {
        fprintf(stderr, "oxdbg: falha ao escrever o registro de eventos\n");
        return 1;
    }
    return s.result;
}
