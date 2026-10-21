/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

/* Alfabeto padrão da RFC 4648. O índice de cada símbolo é o seu valor. */
static const char ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int error(const char *message)
{
    fprintf(stderr, "oxide-base64: %s\n", message);
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

/* Devolve o valor de 0 a 63 de um símbolo, ou -1 fora do alfabeto.
 * strchr encontra o terminador '\0'; por isso o byte zero é rejeitado antes. */
static int sextet(int symbol)
{
    if (symbol <= 0) {
        return -1;
    }
    const char *found = strchr(ALPHABET, symbol);
    return found != NULL ? (int)(found - ALPHABET) : -1;
}

/* Cada grupo de três bytes (24 bits) vira quatro símbolos de seis bits.
 * No último grupo, os bytes ausentes valem zero e os símbolos que só
 * dependeriam deles viram '='. */
static int encode(void)
{
    unsigned char group[3];
    size_t count;

    while ((count = fread(group, 1, sizeof group, stdin)) != 0) {
        const unsigned int bits = (unsigned int)group[0] << 16 |
                                  (count > 1 ? group[1] : 0u) << 8 |
                                  (count > 2 ? group[2] : 0u);
        const char symbols[4] = {
            ALPHABET[(bits >> 18) & 63],
            ALPHABET[(bits >> 12) & 63],
            count > 1 ? ALPHABET[(bits >> 6) & 63] : '=',
            count > 2 ? ALPHABET[bits & 63] : '=',
        };
        if (fwrite(symbols, 1, sizeof symbols, stdout) != sizeof symbols) {
            return error("erro de escrita");
        }
    }
    return ferror(stdin) ? error("erro de leitura") : EXIT_SUCCESS;
}

/* Lê quartetos de símbolos. Cada quarteto completo produz três bytes.
 * O padding só é aceito no último quarteto, nas formas xx== e xxx=. */
static int decode(void)
{
    int first;

    while ((first = getchar()) != EOF) {
        const int symbols[4] = {first, getchar(), getchar(), getchar()};
        if (symbols[3] == EOF) {
            return error(ferror(stdin) ? "erro de leitura"
                                       : "comprimento não múltiplo de quatro");
        }

        /* Os dois primeiros símbolos nunca são padding. */
        const int a = sextet(symbols[0]);
        const int b = sextet(symbols[1]);
        if (a < 0 || b < 0) {
            return error("símbolo fora do alfabeto");
        }

        size_t padding = 0;
        if (symbols[2] == '=') {
            if (symbols[3] != '=') {
                return error("padding em posição inválida");
            }
            padding = 2;
        } else if (symbols[3] == '=') {
            padding = 1;
        }

        const int c = padding == 2 ? 0 : sextet(symbols[2]);
        const int d = padding >= 1 ? 0 : sextet(symbols[3]);
        if (c < 0 || d < 0) {
            return error("símbolo fora do alfabeto");
        }

        /* Os quatro sextetos formam 24 bits. Sem padding, os três bytes
         * são válidos. Com padding, os bits descartados precisam ser zero:
         * essa é a forma canônica exigida pelo desafio. */
        const unsigned int bits = (unsigned int)a << 18 |
                                  (unsigned int)b << 12 | (unsigned int)c << 6 |
                                  (unsigned int)d;
        const unsigned char bytes[3] = {
            (unsigned char)(bits >> 16),
            (unsigned char)(bits >> 8),
            (unsigned char)bits,
        };
        unsigned int discarded = 0;
        if (padding == 2) {
            discarded = bits & 0xffff;
        } else if (padding == 1) {
            discarded = bits & 0xff;
        }
        if (discarded != 0) {
            return error("bits de preenchimento diferentes de zero");
        }

        const size_t produced = sizeof bytes - padding;
        if (fwrite(bytes, 1, produced, stdout) != produced) {
            return error("erro de escrita");
        }
        if (padding != 0) {
            if (getchar() != EOF) {
                return error("dados após o padding");
            }
            break;
        }
    }
    return ferror(stdin) ? error("erro de leitura") : EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc != 2 ||
        (strcmp(argv[1], "encode") != 0 && strcmp(argv[1], "decode") != 0)) {
        return error("uso: oxide-base64 encode|decode");
    }
    ignore_sigpipe();

    const int result = strcmp(argv[1], "encode") == 0 ? encode() : decode();

    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (fflush(stdout) == EOF) {
        return error("erro de escrita");
    }
    return result;
}
