/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* A definição enumera bytes ASCII; não dependemos da locale do processo. */
static bool separator(int byte)
{
    return byte == ' ' || byte == '\t' || byte == '\n' ||
           byte == '\r' || byte == '\v' || byte == '\f';
}

int main(void)
{
    unsigned long long lines = 0;
    unsigned long long words = 0;
    unsigned long long bytes = 0;
    bool in_word = false;
    int byte;

    /* fgetc retorna int: precisamos distinguir qualquer byte do sentinela EOF.
     * Só mantemos contadores e o estado da palavra, não a entrada inteira. */
    while ((byte = fgetc(stdin)) != EOF) {
        ++bytes;
        if (byte == '\n') {
            ++lines;
        }

        if (separator(byte)) {
            in_word = false;
        } else if (!in_word) {
            /* Contar a entrada na palavra também inclui a última palavra
             * quando a entrada termina sem separador. */
            ++words;
            in_word = true;
        }
    }

    if (ferror(stdin)) {
        fputs("oxide-wc: erro de leitura\n", stderr);
        return EXIT_FAILURE;
    }
    if (printf("%llu %llu %llu\n", lines, words, bytes) < 0 ||
        fflush(stdout) == EOF) {
        fputs("oxide-wc: erro de escrita\n", stderr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
