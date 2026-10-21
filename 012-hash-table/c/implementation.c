/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include "api.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Quantidade de slots da primeira reserva. */
#define INITIAL_CAPACITY 8

/* Invariantes:
 * - capacity é zero ou uma potência de dois;
 * - count <= capacity / 2 depois de cada inserção (fator de carga);
 * - todo slot tem occupied igual a 0 ou 1, inicializado antes da consulta.
 *
 * O fator de carga garante slots vazios. Por isso, a sondagem linear sempre
 * termina: ou encontra a chave, ou encontra um slot vazio. */

/* Sondagem linear. Devolve o índice do slot com a chave, ou do primeiro
 * slot vazio na sequência de sondagem.
 *
 * Como capacity é potência de dois, `& (capacity - 1)` é o módulo. O hash
 * público é (uint32_t)key & (capacity - 1). Em colisão, a sondagem avança
 * um slot por vez e volta ao início depois do último. */
static size_t find_slot(const Pair *pairs, size_t capacity, int32_t key)
{
    size_t index = (uint32_t)key & (capacity - 1);

    while (pairs[index].occupied && pairs[index].key != key) {
        index = (index + 1) & (capacity - 1);
    }
    return index;
}

void table_init(Table *table, Allocate allocate)
{
    table->pairs = NULL;
    table->count = 0;
    table->capacity = 0;
    table->allocate = allocate;
}

/* Dobra a capacidade e reinsere cada par com o hash da nova capacidade.
 * Qualquer falha devolve 0 e preserva a tabela antiga. */
static int grow(Table *table)
{
    if (table->capacity > SIZE_MAX / 2) {
        return 0;
    }
    const size_t capacity =
        table->capacity != 0 ? table->capacity * 2 : INITIAL_CAPACITY;
    if (capacity > SIZE_MAX / sizeof(Pair)) {
        return 0;
    }

    Pair *pairs = table->allocate(capacity * sizeof *pairs);
    if (pairs == NULL) {
        return 0;
    }
    /* Todos os slots começam vazios antes de qualquer consulta. */
    memset(pairs, 0, capacity * sizeof *pairs);

    /* O hash muda com a capacidade. Cada par recebe uma posição nova;
     * copiar os slots para os mesmos índices quebraria as consultas. */
    for (size_t i = 0; i < table->capacity; ++i) {
        if (table->pairs[i].occupied) {
            const size_t index =
                find_slot(pairs, capacity, table->pairs[i].key);
            pairs[index] = table->pairs[i];
        }
    }

    free(table->pairs);
    table->pairs = pairs;
    table->capacity = capacity;
    return 1;
}

int table_put(Table *table, int32_t key, int32_t value)
{
    /* Atualização: a chave já existe. Não conta nem aloca. */
    if (table->capacity != 0) {
        const size_t index = find_slot(table->pairs, table->capacity, key);
        if (table->pairs[index].occupied) {
            table->pairs[index].value = value;
            return 1;
        }
    }

    /* Chave nova: a tabela cresce antes de a carga chegar à metade. */
    if (table->capacity == 0 || table->count >= table->capacity / 2) {
        if (!grow(table)) {
            return 0;
        }
    }

    const size_t index = find_slot(table->pairs, table->capacity, key);
    table->pairs[index] = (Pair){key, value, 1};
    table->count++;
    return 1;
}

int table_get(const Table *table, int32_t key, int32_t *value)
{
    if (table->capacity == 0) {
        return 0;
    }
    const size_t index = find_slot(table->pairs, table->capacity, key);
    if (!table->pairs[index].occupied) {
        return 0;
    }
    *value = table->pairs[index].value;
    return 1;
}

void table_destroy(Table *table)
{
    free(table->pairs);
    table->pairs = NULL;
    table->count = 0;
    table->capacity = 0;
}
