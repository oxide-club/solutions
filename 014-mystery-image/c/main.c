/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

/* Especificação inferida do formato .px (veja o README):
 *
 *   offset  tamanho  conteúdo
 *   0       2        assinatura "PX"
 *   2       1        versão, sempre 1
 *   3       1        reservado, sempre 0
 *   4       2        largura, u16 big endian
 *   6       2        altura, u16 big endian
 *   8       3·w·h    pixels BGR, em ordem de colunas
 *
 * "Ordem de colunas" significa que o arquivo percorre primeiro y e depois
 * x: o pixel (x, y) fica em 8 + 3·(x·h + y). Não há bytes finais. */
#define INPUT_LIMIT 65536
#define HEADER_SIZE 8
#define DIMENSION_LIMIT 64

/* A entrada inteira fica na memória: o tamanho exato só pode ser conferido
 * depois de ler o cabeçalho. O byte extra detecta entrada acima do limite. */
static unsigned char data[INPUT_LIMIT + 1];
static size_t length;

static int error(const char *message)
{
    fprintf(stderr, "oxide-mystery-image: %s\n", message);
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

/* Inteiro sem sinal de 16 bits em big endian: o primeiro byte é o mais
 * significativo. A consulta tall (1×2) mostrou os bytes 00 01 00 02. */
static unsigned int u16be(size_t offset)
{
    return ((unsigned int)data[offset] << 8) | data[offset + 1];
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        return error("uso: oxide-mystery-image < imagem.px > imagem.ppm");
    }
    ignore_sigpipe();

    length = fread(data, 1, sizeof data, stdin);
    if (ferror(stdin)) {
        return error("erro de leitura");
    }
    if (length > INPUT_LIMIT) {
        return error("entrada maior que 65536 bytes");
    }

    if (length < HEADER_SIZE || memcmp(data, "PX\1\0", 4) != 0) {
        return error("cabeçalho PX inválido");
    }
    const unsigned int width = u16be(4);
    const unsigned int height = u16be(6);
    if (width == 0 || height == 0 || width > DIMENSION_LIMIT ||
        height > DIMENSION_LIMIT) {
        return error("dimensões fora de 1..64");
    }
    /* O tamanho precisa ser exato: payload incompleto e bytes finais são
     * rejeitados pela mesma comparação. */
    if (length != HEADER_SIZE + 3 * (size_t)width * height) {
        return error("tamanho do arquivo não corresponde às dimensões");
    }

    printf("P6\n%u %u\n255\n", width, height);

    /* A saída PPM percorre as linhas: y externo, x interno. O arquivo .px
     * percorre as colunas: por isso o índice de origem é x·h + y.
     * Cada pixel vem como B, G, R e sai como R, G, B. */
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            const size_t source = HEADER_SIZE + 3 * ((size_t)x * height + y);
            const unsigned char rgb[3] = {
                data[source + 2],
                data[source + 1],
                data[source],
            };
            if (fwrite(rgb, 1, sizeof rgb, stdout) != sizeof rgb) {
                return error("erro de escrita");
            }
        }
    }

    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (ferror(stdout) || fflush(stdout) == EOF) {
        return error("erro de escrita");
    }
    return EXIT_SUCCESS;
}
