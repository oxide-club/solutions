/* SPDX-License-Identifier: MIT */
#define _GNU_SOURCE
#include "common.h"
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct timespec ox_deadline(void) {
    struct timespec deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) != 0) {
        /* CLOCK_MONOTONIC existe no ambiente Linux exigido. */
        deadline = (struct timespec){0, 0};
    }
    deadline.tv_sec += 2;
    return deadline;
}

int ox_wait(int fd, short events, const struct timespec *deadline) {
    for (;;) {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            return -1;
        }
        long long ns = (long long)(deadline->tv_sec - now.tv_sec) * 1000000000LL +
                       deadline->tv_nsec - now.tv_nsec;
        if (ns <= 0) {
            return -2;
        }
        int milliseconds = (int)((ns + 999999) / 1000000);
        struct pollfd pending = {fd, events, 0};
        int result = poll(&pending, 1, milliseconds);
        if (result > 0) {
            return 0;
        }
        if (result == 0) {
            return -2;
        }
        if (errno != EINTR) {
            return -1;
        }
    }
}

int ox_write(int fd, const void *data, size_t length, const struct timespec *deadline) {
    const unsigned char *bytes = data;
    size_t sent = 0;
    while (sent < length) {
        int result = ox_wait(fd, POLLOUT, deadline);
        if (result != 0) {
            return result;
        }
        ssize_t count = write(fd, bytes + sent, length - sent);
        if (count > 0) {
            sent += (size_t)count;
        } else if (count < 0 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        } else {
            return -1;
        }
    }
    return 0;
}

int ox_line(int fd, char line[OX_LINE + 1], const struct timespec *deadline) {
    for (size_t used = 0; used < OX_LINE; ++used) {
        unsigned char byte;
        for (;;) {
            int result = ox_wait(fd, POLLIN, deadline);
            if (result != 0) {
                return result;
            }
            ssize_t count = read(fd, &byte, 1);
            if (count == 1) {
                break;
            }
            if (count < 0 && (errno == EINTR || errno == EAGAIN)) {
                continue;
            }
            return -1;
        }
        if (byte == '\n') {
            line[used] = '\0';
            return 0;
        }
        if (byte < 32 || byte > 126) {
            return -3;
        }
        line[used] = (char)byte;
    }
    return -3;
}

int ox_send(int fd, const char *line, int resource, const struct timespec *deadline) {
    if (resource < 0) {
        return ox_write(fd, line, strlen(line), deadline);
    }
    union {
        struct cmsghdr align;
        unsigned char bytes[CMSG_SPACE(sizeof(int))];
    } control = {0};
    struct iovec payload = {(void *)line, strlen(line)};
    struct msghdr message = {0};
    message.msg_iov = &payload;
    message.msg_iovlen = 1;
    message.msg_control = control.bytes;
    message.msg_controllen = sizeof(control.bytes);
    struct cmsghdr *header = CMSG_FIRSTHDR(&message);
    header->cmsg_level = SOL_SOCKET;
    header->cmsg_type = SCM_RIGHTS;
    header->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(header), &resource, sizeof(resource));
    for (;;) {
        int result = ox_wait(fd, POLLOUT, deadline);
        if (result != 0) {
            return result;
        }
        ssize_t count = sendmsg(fd, &message, MSG_NOSIGNAL);
        if (count > 0) {
            /* O descritor já foi entregue; não o repita no restante. */
            return ox_write(fd, line + count, strlen(line) - (size_t)count, deadline);
        }
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        }
        return -1;
    }
}

int ox_hello(int fd, char line[OX_LINE + 1], int *resource, const struct timespec *deadline) {
    size_t received = 0;
    *resource = -1;
    for (size_t used = 0; used < OX_LINE; ++used) {
        union {
            struct cmsghdr align;
            unsigned char bytes[CMSG_SPACE(16 * sizeof(int))];
        } control;
        unsigned char byte;
        struct iovec payload = {&byte, 1};
        struct msghdr message = {0};
        message.msg_iov = &payload;
        message.msg_iovlen = 1;
        message.msg_control = control.bytes;
        message.msg_controllen = sizeof(control.bytes);
        ssize_t count;
        for (;;) {
            int result = ox_wait(fd, POLLIN, deadline);
            if (result != 0) {
                return result;
            }
            count = recvmsg(fd, &message, MSG_CMSG_CLOEXEC);
            if (count >= 0 || (errno != EINTR && errno != EAGAIN)) {
                break;
            }
        }
        if (count <= 0) {
            return -1;
        }
        for (struct cmsghdr *header = CMSG_FIRSTHDR(&message); header != NULL;
             header = CMSG_NXTHDR(&message, header)) {
            if (header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_RIGHTS) {
                size_t length = header->cmsg_len - CMSG_LEN(0);
                for (size_t offset = 0; offset + sizeof(int) <= length; offset += sizeof(int)) {
                    int descriptor;
                    memcpy(&descriptor, CMSG_DATA(header) + offset, sizeof(descriptor));
                    if (received++ == 0) {
                        *resource = descriptor;
                    } else {
                        close(descriptor);
                    }
                }
            }
        }
        if (message.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) {
            return -3;
        }
        if (byte == '\n') {
            line[used] = '\0';
            return received == 1 ? 0 : -3;
        }
        if (byte < 32 || byte > 126) {
            return -3;
        }
        line[used] = (char)byte;
    }
    return -3;
}

int ox_fields(char *line, char **fields, size_t capacity) {
    size_t count = 0;
    char *start = line;
    for (char *cursor = line;; ++cursor) {
        if (*cursor == ' ' || *cursor == '\0') {
            if (cursor == start || count == capacity) {
                return -1;
            }
            int last = *cursor == '\0';
            *cursor = '\0';
            fields[count++] = start;
            if (last) {
                return (int)count;
            }
            start = cursor + 1;
        }
    }
}

int ox_uint(const char *text, uint64_t *value) {
    if (*text == '\0' || (text[0] == '0' && text[1] != '\0')) {
        return -1;
    }
    uint64_t result = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p < '0' || *p > '9' || result > (UINT64_MAX - (*p - '0')) / 10) {
            return -1;
        }
        result = result * 10 + (*p - '0');
    }
    *value = result;
    return 0;
}

int ox_lock(struct ox_region *region, const struct timespec *deadline) {
    int result = pthread_mutex_clocklock(&region->mutex, CLOCK_MONOTONIC, deadline);
    if (result == EOWNERDEAD) {
        /* Recebemos ownership, mas não temos prova de consistência. */
        pthread_mutex_unlock(&region->mutex);
    }
    return result;
}

int ox_valid(const struct ox_region *region) {
    return region->version == 1 && region->counter <= OX_LIMIT &&
           region->counter_xor == (region->counter ^ UINT64_MAX);
}

struct ox_state ox_copy(const struct ox_region *region) {
    return (struct ox_state){region->generation, region->counter, region->counter_xor};
}
