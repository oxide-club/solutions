/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include "api.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Tamanho da primeira reserva, em bytes. */
#define INITIAL_CAPACITY 8

/* Invariantes: size <= capacity; data == NULL somente com capacity == 0.
 * A reserva vem do callback resize e é compatível com free. */

void buffer_init(Buffer *buffer, Resize resize)
{
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
    buffer->resize = resize;
}

/* Calcula a nova capacidade para guardar `needed` bytes.
 * A primeira reserva tem INITIAL_CAPACITY bytes. Depois, a capacidade
 * dobra até caber. Se a próxima duplicação passaria de SIZE_MAX, a função
 * devolve exatamente o tamanho necessário. */
static size_t grown_capacity(size_t current, size_t needed)
{
    size_t capacity = current != 0 ? current : INITIAL_CAPACITY;

    while (capacity < needed) {
        if (capacity > SIZE_MAX / 2) {
            return needed;
        }
        capacity *= 2;
    }
    return capacity;
}

int buffer_append(Buffer *buffer, const void *source, size_t n)
{
    /* Overflow de size + n: rejeitado antes de ler a origem e antes de
     * chamar o alocador. O teste direto confere essa ordem. */
    if (n > SIZE_MAX - buffer->size) {
        return 0;
    }
    const size_t needed = buffer->size + n;

    if (needed > buffer->capacity) {
        const size_t capacity = grown_capacity(buffer->capacity, needed);
        void *grown = buffer->resize(buffer->data, capacity);

        /* Contrato de realloc: NULL indica falha e a região antiga continua
         * válida. O buffer só publica a nova reserva depois do sucesso.
         * Assim, uma falha preserva ponteiro, conteúdo, tamanho e
         * capacidade. */
        if (grown == NULL) {
            return 0;
        }
        buffer->data = grown;
        buffer->capacity = capacity;
    }

    /* Com n = 0, a origem pode ser NULL; memcpy não é chamado. */
    if (n != 0) {
        memcpy(buffer->data + buffer->size, source, n);
    }
    buffer->size = needed;
    return 1;
}

/* Libera a reserva e volta ao estado inicial. */
void buffer_destroy(Buffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}
