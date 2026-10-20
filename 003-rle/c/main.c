/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

static int error(const char *message)
{
    fprintf(stderr, "oxide-rle: %s\n", message);
    return EXIT_FAILURE;
}

static int pair(unsigned int count, int value)
{
    return putchar((int)count) != EOF && putchar(value) != EOF;
}

static int encode(void)
{
    int value = 0;
    unsigned int count = 0;
    int byte;

    /* getchar retorna int: todos os bytes, inclusive 0xff, diferem de EOF.
     * O estado sobrevive às leituras; nenhum arquivo inteiro fica na memória. */
    while ((byte = getchar()) != EOF) {
        if (count != 0 && (byte != value || count == 255)) {
            if (!pair(count, value)) {
                return error("erro de escrita");
            }
            count = 0;
        }
        value = byte;
        ++count;
    }
    if (ferror(stdin)) {
        return error("erro de leitura");
    }
    if (count != 0 && !pair(count, value)) {
        return error("erro de escrita");
    }
    return EXIT_SUCCESS;
}

static int decode(void)
{
    int count;
    while ((count = getchar()) != EOF) {
        if (count == 0) {
            return error("contagem zero");
        }
        const int value = getchar();
        if (value == EOF) {
            return error(ferror(stdin) ? "erro de leitura" : "par incompleto");
        }
        /* Pares adjacentes com valores iguais também são válidos. */
        for (int i = 0; i < count; ++i) {
            if (putchar(value) == EOF) {
                return error("erro de escrita");
            }
        }
    }
    return ferror(stdin) ? error("erro de leitura") : EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc != 2 ||
        (strcmp(argv[1], "encode") != 0 && strcmp(argv[1], "decode") != 0)) {
        return error("uso: oxide-rle encode|decode");
    }
#ifdef SIGPIPE
    /* Unix pode encerrar o processo antes de stdio reportar um pipe fechado.
     * Ignorar esse sinal permite diagnosticar a falha de escrita normalmente.
     * SIGPIPE é uma extensão do ambiente, não um requisito do C11. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        return error("erro ao configurar SIGPIPE");
    }
#endif
    const int result = strcmp(argv[1], "encode") == 0 ? encode() : decode();
    /* Uma escrita aceita por stdio pode falhar somente no flush final. */
    if (fflush(stdout) == EOF) {
        return error("erro de escrita");
    }
    return result;
}
