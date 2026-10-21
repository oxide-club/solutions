/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

/* Tamanho de cada bloco lido de stdin. O estado da busca sobrevive entre
 * blocos, então uma ocorrência pode começar em um bloco e terminar no
 * seguinte. */
#define BLOCK_SIZE 4096

/* O padrão decodificado ocupa até 1 MiB; o argumento tem o dobro em
 * dígitos hexadecimais. */
#define PATTERN_LIMIT 1048576

static int error(const char *message)
{
    fprintf(stderr, "oxide-stream-search: %s\n", message);
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

/* Devolve o valor de um dígito hexadecimal, ou -1 se o byte não é dígito. */
static int hex_digit(int c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/* Converte 2 × length dígitos em length bytes. Devolve 0 se um dígito é
 * inválido. */
static int parse_pattern(const char *text, unsigned char *pattern,
                         size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        const int high = hex_digit(text[2 * i]);
        const int low = hex_digit(text[2 * i + 1]);
        if (high < 0 || low < 0) {
            return 0;
        }
        pattern[i] = (unsigned char)(high * 16 + low);
    }
    return 1;
}

/* Tabela de prefixos do KMP. prefix[i] é o tamanho do maior prefixo
 * próprio de pattern[0..i] que também é sufixo desse trecho.
 *
 * Depois de uma falha na posição j, o algoritmo recua para prefix[j - 1]
 * em vez de voltar ao início. Os bytes já comparados continuam casados com
 * esse prefixo menor, então nenhum byte da entrada é lido duas vezes.
 *
 * A construção aplica a mesma ideia ao próprio padrão. */
static void build_prefix_table(const unsigned char *pattern, size_t length,
                               size_t *prefix)
{
    size_t matched = 0;

    prefix[0] = 0;
    for (size_t i = 1; i < length; ++i) {
        while (matched > 0 && pattern[i] != pattern[matched]) {
            matched = prefix[matched - 1];
        }
        if (pattern[i] == pattern[matched]) {
            ++matched;
        }
        prefix[i] = matched;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return error("uso: oxide-stream-search PADRAO_HEX");
    }
    ignore_sigpipe();

    const size_t digits = strlen(argv[1]);
    if (digits == 0 || digits % 2 != 0 || digits > 2 * PATTERN_LIMIT) {
        return error("padrão vazio, ímpar ou maior que 1 MiB");
    }
    const size_t length = digits / 2;

    unsigned char *pattern = malloc(length);
    size_t *prefix = malloc(length * sizeof *prefix);
    int status = EXIT_FAILURE;

    if (pattern == NULL || prefix == NULL) {
        error("falha de alocação");
        goto cleanup;
    }
    if (!parse_pattern(argv[1], pattern, length)) {
        error("dígito hexadecimal inválido");
        goto cleanup;
    }
    build_prefix_table(pattern, length, prefix);

    /* Estado da busca: quantos bytes do padrão já casaram (matched) e a
     * posição do próximo byte da entrada (position). Os dois sobrevivem
     * entre blocos e entre ocorrências. */
    unsigned char block[BLOCK_SIZE];
    size_t matched = 0;
    uint64_t position = 0;
    size_t count;

    while ((count = fread(block, 1, sizeof block, stdin)) != 0) {
        for (size_t i = 0; i < count; ++i) {
            while (matched > 0 && block[i] != pattern[matched]) {
                matched = prefix[matched - 1];
            }
            if (block[i] == pattern[matched]) {
                ++matched;
            }
            if (matched == length) {
                /* O padrão termina em position; o início é length - 1
                 * bytes antes. O recuo pela tabela permite ocorrências
                 * sobrepostas. */
                const uint64_t start = position + 1 - length;
                printf("%llu\n", (unsigned long long)start);
                matched = prefix[matched - 1];
            }
            if (position == UINT64_MAX) {
                error("entrada excede uint64_t");
                goto cleanup;
            }
            ++position;
        }
    }

    if (ferror(stdin)) {
        error("erro de leitura");
    } else if (ferror(stdout) || fflush(stdout) == EOF) {
        /* stdio pode guardar a saída no buffer e só falhar no flush final. */
        error("erro de escrita");
    } else {
        status = EXIT_SUCCESS;
    }

cleanup:
    free(pattern);
    free(prefix);
    return status;
}
