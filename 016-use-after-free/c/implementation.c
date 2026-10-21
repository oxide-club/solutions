/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include "api.h"

#include <stdlib.h>
#include <string.h>

/* Contrato de propriedade: a origem pertence ao chamador e não é alterada.
 * A cópia passa a pertencer ao chamador, que a libera depois do último uso.
 * Esta função nunca chama free: o starter liberava a região antes de
 * devolvê-la, e o chamador lia memória já liberada. */
unsigned char *copy_bytes(const unsigned char *source, size_t n)
{
    /* malloc(0) pode devolver NULL em algumas implementações. Um byte
     * garante que a cópia vazia também é uma alocação liberável. */
    unsigned char *copy = malloc(n != 0 ? n : 1);
    if (copy == NULL) {
        return NULL;
    }

    /* Com n = 0, a origem pode ser NULL; memcpy não é chamado. */
    if (n != 0) {
        memcpy(copy, source, n);
    }
    return copy;
}
