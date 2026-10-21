/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include "api.h"

#include <stdint.h>

/* O mapa é o array Pool.blocks, com count descritores ordenados por
 * offset. Invariantes depois de cada operação:
 * - os blocos cobrem a capacidade útil sem lacunas nem sobreposição;
 * - todo offset e todo tamanho são múltiplos de BLOCK_ALIGNMENT;
 * - todo tamanho é positivo;
 * - dois blocos livres nunca são vizinhos (coalescência).
 * Os descritores ficam fora da região e não consomem capacidade. */

/* Arredonda para cima ao múltiplo de BLOCK_ALIGNMENT. O chamador já
 * conferiu que a soma não transborda. */
static size_t round_up(size_t n)
{
    return (n + BLOCK_ALIGNMENT - 1) & ~(size_t)(BLOCK_ALIGNMENT - 1);
}

void pool_init(Pool *pool, void *base, size_t capacity)
{
    pool->base = base;
    /* A capacidade útil é um múltiplo do alinhamento. Os bytes restantes
     * no fim da região nunca são entregues. */
    pool->capacity = capacity - capacity % BLOCK_ALIGNMENT;
    pool->count = pool->capacity != 0 ? 1 : 0;
    if (pool->count != 0) {
        pool->blocks[0] = (Block){0, pool->capacity, 0};
    }
}

/* First-fit: percorre o mapa em ordem e escolhe o primeiro bloco livre com
 * tamanho suficiente. Se sobra espaço, divide o bloco em dois: o primeiro
 * fica usado e o segundo fica livre. Tamanho inválido, overflow, falta de
 * espaço ou falta de descritor devolve NULL sem alterar o mapa. */
void *pool_alloc(Pool *pool, size_t n)
{
    if (n == 0 || n > SIZE_MAX - (BLOCK_ALIGNMENT - 1)) {
        return NULL;
    }
    n = round_up(n);

    for (size_t i = 0; i < pool->count; ++i) {
        Block *block = &pool->blocks[i];
        if (block->used || block->size < n) {
            continue;
        }

        if (block->size > n) {
            /* A sobra é múltipla de 8, então tem no mínimo 8 bytes e vira
             * um bloco livre novo logo depois deste. O novo descritor
             * precisa de uma vaga no array. */
            if (pool->count == BLOCK_LIMIT) {
                return NULL;
            }
            /* Abre espaço na posição i + 1 deslocando os descritores
             * seguintes uma posição para a direita. */
            for (size_t j = pool->count; j > i + 1; --j) {
                pool->blocks[j] = pool->blocks[j - 1];
            }
            pool->blocks[i + 1] =
                (Block){block->offset + n, block->size - n, 0};
            pool->count++;
            block->size = n;
        }

        block->used = 1;
        return pool->base + block->offset;
    }
    return NULL;
}

/* Marca o bloco como livre e une todos os blocos livres vizinhos.
 * Só ponteiros vivos entregues por este pool são válidos aqui. */
void pool_free(Pool *pool, void *pointer)
{
    const size_t offset = (size_t)((unsigned char *)pointer - pool->base);

    size_t i = 0;
    while (i < pool->count && pool->blocks[i].offset != offset) {
        ++i;
    }
    if (i == pool->count) {
        return;
    }
    pool->blocks[i].used = 0;

    /* Coalescência: enquanto dois vizinhos estão livres, o primeiro absorve
     * o segundo e os descritores seguintes deslocam uma posição para a
     * esquerda. Um único percurso basta, porque o bloco recém-liberado é o
     * único que pode ter criado vizinhos livres. */
    i = 0;
    while (i + 1 < pool->count) {
        if (!pool->blocks[i].used && !pool->blocks[i + 1].used) {
            pool->blocks[i].size += pool->blocks[i + 1].size;
            for (size_t j = i + 1; j + 1 < pool->count; ++j) {
                pool->blocks[j] = pool->blocks[j + 1];
            }
            pool->count--;
        } else {
            ++i;
        }
    }
}
