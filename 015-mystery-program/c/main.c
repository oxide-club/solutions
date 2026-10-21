/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ATENÇÃO: este arquivo também é a fonte do alvo distribuído pelo desafio.
 * rebuild-target.sh compila main.c com -O0 e confere os offsets citados em
 * materials/traces.md. Comentários e nomes não alteram o código gerado.
 * Mudanças na ordem das declarações ou nas instruções alteram os offsets.
 * Depois de qualquer edição, execute o script e confira o resultado. */

/* Transformação reconstruída (veja o README):
 *   out[n - 1 - i] = ((in[i] XOR 0x5a) + i) mod 256
 * Cada byte é alterado pela posição original e a sequência é invertida. */

static unsigned char data[65537];
static size_t len;

static void fail(void)
{
    fputs("entrada ou operação inválida\n", stderr);
    exit(1);
}

static void output(const void *p, size_t n)
{
    if (fwrite(p, 1, n, stdout) != n) {
        fail();
    }
}

static void finish(void)
{
    if (fflush(stdout) || ferror(stdout)) {
        fail();
    }
}

/* Um byte extra distingue o limite aceito de uma entrada excessiva. */
static void readall(void)
{
    len = fread(data, 1, sizeof(data), stdin);
    if (ferror(stdin) || len > 65536) {
        fail();
    }
}

int main(int argc, char **argv)
{
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
    (void)argv;
    if (argc != 1) {
        fail();
    }
    readall();

    /* O enunciado limita a entrada a 64 bytes. O laço abaixo só acessa
     * out[0..len-1], então 65 ou mais bytes são rejeitados antes. */
    if (len > 64) {
        fail();
    }

    unsigned char out[64];
    for (size_t i = 0; i < len; i++) {
        /* XOR com 0x5a: 00 vira 5a, como no primeiro trace.
         * Soma de i: os três zeros viram 5a, 5b, 5c antes da inversão.
         * Índice len - 1 - i: a saída fica invertida, 5c 5b 5a.
         * A conversão para unsigned char aplica o módulo 256. */
        out[len - 1 - i] = (unsigned char)((data[i] ^ 0x5a) + i);
    }

    output(out, len);
    finish();
    return 0;
}
