/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

/* Limites e tamanhos fixos do formato REC1. */
#define INPUT_LIMIT 65536
#define FILE_HEADER_SIZE 6   /* "REC1" + contagem u16 */
#define RECORD_HEADER_SIZE 3 /* tipo u8 + comprimento u16 */

/* A entrada inteira fica na memória: o formato exige validar o fim exato
 * antes de escrever qualquer linha. O byte extra permite detectar uma
 * entrada maior que o limite sem contar além dele. */
static unsigned char data[INPUT_LIMIT + 1];
static size_t length;

static int error(const char *message)
{
    fprintf(stderr, "oxide-record-inspector: %s\n", message);
    return EXIT_FAILURE;
}

/* Um pipe sem leitor gera SIGPIPE e encerra o processo em silêncio.
 * Com o sinal ignorado, a escrita falha e o programa emite o diagnóstico. */
static void ignore_sigpipe(void)
{
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
}

/* Inteiro sem sinal de 16 bits em little endian: o primeiro byte é o menos
 * significativo. O chamador garante que data[offset + 1] existe. */
static unsigned int u16le(size_t offset)
{
    return data[offset] | ((unsigned int)data[offset + 1] << 8);
}

/* Primeira passagem: confere cabeçalho, cada registro e o fim exato.
 * Devolve 1 se o arquivo é consistente. Nada é escrito aqui.
 *
 * Cada acesso compara o que falta ler com o que resta no buffer, usando a
 * subtração length - offset. A soma offset + n poderia transbordar. */
static int validate(void)
{
    if (length < FILE_HEADER_SIZE || memcmp(data, "REC1", 4) != 0) {
        return 0;
    }
    const unsigned int count = u16le(4);
    size_t offset = FILE_HEADER_SIZE;

    for (unsigned int i = 0; i < count; ++i) {
        if (length - offset < RECORD_HEADER_SIZE) {
            return 0;
        }
        const unsigned int payload = u16le(offset + 1);
        offset += RECORD_HEADER_SIZE;
        if (payload > length - offset) {
            return 0;
        }
        offset += payload;
    }
    /* A contagem corresponde exatamente aos registros presentes. */
    return offset == length;
}

/* Segunda passagem: escreve o relatório. Os limites já foram conferidos. */
static void report(void)
{
    const unsigned int count = u16le(4);
    size_t offset = FILE_HEADER_SIZE;

    printf("count=%u\n", count);
    for (unsigned int i = 0; i < count; ++i) {
        const unsigned int type = data[offset];
        const unsigned int payload = u16le(offset + 1);

        /* O offset aponta para o byte do tipo, como o enunciado define. */
        printf("offset=%zu type=%u length=%u data=", offset, type, payload);
        offset += RECORD_HEADER_SIZE;
        for (unsigned int j = 0; j < payload; ++j) {
            printf("%02x", data[offset + j]);
        }
        putchar('\n');
        offset += payload;
    }
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        return error("uso: oxide-record-inspector < arquivo.rec");
    }
    ignore_sigpipe();

    length = fread(data, 1, sizeof data, stdin);
    if (ferror(stdin)) {
        return error("erro de leitura");
    }
    if (length > INPUT_LIMIT) {
        return error("entrada maior que 65536 bytes");
    }

    if (!validate()) {
        return error("arquivo REC1 truncado ou inconsistente");
    }
    report();

    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (ferror(stdout) || fflush(stdout) == EOF) {
        return error("erro de escrita");
    }
    return EXIT_SUCCESS;
}
