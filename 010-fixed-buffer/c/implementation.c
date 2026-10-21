/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include "api.h"

#include <string.h>

/* Invariante da estrutura: 0 <= size <= capacity.
 *
 * size é a quantidade de bytes em uso. capacity é o espaço total da região.
 * A região pertence ao chamador: esta API nunca aloca nem libera memória. */

void buffer_init(Buffer *buffer, unsigned char *region, size_t capacity)
{
    buffer->data = region;
    buffer->size = 0;
    buffer->capacity = capacity;
}

/* Acrescenta n bytes e devolve 1. Se os bytes não cabem, devolve 0 e não
 * altera nada.
 *
 * A verificação usa a subtração capacity - size, que nunca transborda
 * porque size <= capacity. A soma size + n poderia passar de SIZE_MAX.
 *
 * A origem só é lida depois da verificação. Com n = 0, a origem pode ser
 * NULL, e memcpy não é chamado. */
int buffer_append(Buffer *buffer, const void *source, size_t n)
{
    if (n > buffer->capacity - buffer->size) {
        return 0;
    }
    if (n != 0) {
        memcpy(buffer->data + buffer->size, source, n);
    }
    buffer->size += n;
    return 1;
}

/* Zera só o tamanho lógico. Os bytes antigos continuam na região, mas
 * deixam de fazer parte do conteúdo. A capacidade não muda. */
void buffer_clear(Buffer *buffer)
{
    buffer->size = 0;
}
