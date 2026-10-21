/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include "api.h"

#include <stdint.h>

/* Maior alinhamento aceito. A base da região já vem alinhada a 16 bytes,
 * então um offset múltiplo do alinhamento produz um endereço alinhado. */
#define ALIGNMENT_LIMIT 16

/* Invariantes: used <= capacity; cada trecho entregue começa em um offset
 * múltiplo do alinhamento pedido; dois trechos entregues desde o último
 * reset nunca se sobrepõem. A região pertence ao chamador. */

void arena_init(Arena *arena, void *base, size_t capacity)
{
    arena->base = base;
    arena->used = 0;
    arena->capacity = capacity;
}

/* Devolve base + offset, com offset o menor múltiplo do alinhamento que é
 * maior ou igual a used. Avança used até o fim dos n bytes.
 * Solicitação inválida ou sem espaço devolve NULL e preserva used. */
void *arena_alloc(Arena *arena, size_t n, size_t alignment)
{
    /* Alinhamento válido: 1, 2, 4, 8 ou 16. Uma potência de dois tem um
     * único bit ligado, então alignment & (alignment - 1) é zero. */
    const int power_of_two = (alignment & (alignment - 1)) == 0;
    if (n == 0 || alignment == 0 || alignment > ALIGNMENT_LIMIT ||
        !power_of_two) {
        return NULL;
    }

    /* Preenchimento até o próximo múltiplo. used & (alignment - 1) é o
     * resto da divisão; o complemento, reduzido pela mesma máscara, é
     * zero quando used já está alinhado. */
    const size_t padding =
        (alignment - (arena->used & (alignment - 1))) & (alignment - 1);

    /* As duas verificações usam subtração. used + padding + n poderia
     * passar de SIZE_MAX, como no comando `o`, e parecer pequeno. */
    if (padding > arena->capacity - arena->used) {
        return NULL;
    }
    const size_t start = arena->used + padding;
    if (n > arena->capacity - start) {
        return NULL;
    }

    arena->used = start + n;
    return arena->base + start;
}

/* Zera used. Todos os trechos entregues antes ficam inválidos, mesmo que
 * uma nova alocação devolva o mesmo endereço. */
void arena_reset(Arena *arena)
{
    arena->used = 0;
}
