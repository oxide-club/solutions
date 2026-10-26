/* SPDX-License-Identifier: MIT */
#ifndef OX_REGION_H
#define OX_REGION_H
#include <stdint.h>
#include <pthread.h>
/* ABI local: mesma libc, compilador e arquitetura; não é formato persistente. */
struct ox_region {
    uint64_t version, generation, counter, counter_xor;
    pthread_mutex_t mutex;
};
#endif
