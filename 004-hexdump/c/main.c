/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

/* Cada linha da saída mostra no máximo este número de bytes. */
#define BYTES_PER_LINE 16

static int error(const char *message)
{
    fprintf(stderr, "oxide-hexdump: %s\n", message);
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

/* Escreve uma linha: offset, coluna hexadecimal e coluna ASCII.
 * `count` fica entre 1 e BYTES_PER_LINE. As posições ausentes recebem três
 * espaços, para que a coluna ASCII fique alinhada na última linha.
 * As falhas de escrita ficam registradas em stdout; main confere no fim. */
static void write_line(uint64_t offset, const unsigned char *bytes,
                       size_t count)
{
    printf("%08llx  ", (unsigned long long)offset);

    for (size_t i = 0; i < BYTES_PER_LINE; ++i) {
        if (i < count) {
            printf("%02x ", bytes[i]);
        } else {
            fputs("   ", stdout);
        }
    }

    fputs(" |", stdout);
    for (size_t i = 0; i < count; ++i) {
        /* Só os bytes de 0x20 a 0x7e são caracteres ASCII visíveis. */
        const int visible = bytes[i] >= 0x20 && bytes[i] <= 0x7e;
        putchar(visible ? bytes[i] : '.');
    }
    puts("|");
}

int main(int argc, char **argv)
{
    if (argc > 2) {
        return error("uso: oxide-hexdump [arquivo]");
    }
    ignore_sigpipe();

    /* Sem argumento, a entrada é stdin. O modo "rb" evita conversões de
     * texto em plataformas que as aplicam. */
    FILE *input = stdin;
    if (argc == 2) {
        input = fopen(argv[1], "rb");
        if (input == NULL) {
            return error("não foi possível abrir o arquivo");
        }
    }

    unsigned char bytes[BYTES_PER_LINE];
    uint64_t offset = 0;
    size_t count;

    /* fread só devolve menos de 16 bytes no fim da entrada ou em erro.
     * Assim, apenas a última linha pode ficar incompleta. */
    while ((count = fread(bytes, 1, sizeof bytes, input)) != 0) {
        write_line(offset, bytes, count);
        if (count > UINT64_MAX - offset) {
            return error("offset excede uint64_t");
        }
        offset += count;
    }

    int failed = ferror(input);
    if (input != stdin && fclose(input) == EOF) {
        failed = 1;
    }
    if (failed) {
        return error("erro de leitura");
    }

    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (ferror(stdout) || fflush(stdout) == EOF) {
        return error("erro de escrita");
    }
    return EXIT_SUCCESS;
}
