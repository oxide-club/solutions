/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ppm.h é fornecido pelo desafio em materials/. O Makefile aponta o
 * diretório com -I. O header define Image, ppm_read e ppm_write. */
#include "ppm.h"

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

/* Maior valor aceito para X, Y, W e H. Também é a maior dimensão PPM. */
#define DIMENSION_LIMIT 256

static int error(const char *message)
{
    fprintf(stderr, "oxide-image-transform: %s\n", message);
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

/* Lê um inteiro decimal sem sinal de 0 a DIMENSION_LIMIT.
 * Devolve 0 para texto vazio, sinal, letra ou valor acima do limite.
 * A verificação a cada dígito também impede overflow em textos longos. */
static int parse_number(const char *text, size_t *value)
{
    size_t number = 0;

    if (*text == '\0') {
        return 0;
    }
    for (; *text != '\0'; ++text) {
        if (*text < '0' || *text > '9') {
            return 0;
        }
        number = number * 10 + (size_t)(*text - '0');
        if (number > DIMENSION_LIMIT) {
            return 0;
        }
    }
    *value = number;
    return 1;
}

/* Offset do primeiro canal do pixel (x, y). O stride é a largura. */
static size_t pixel_offset(const Image *image, size_t x, size_t y)
{
    return (y * image->w + x) * 3;
}

/* Mapeamento inverso: cada pixel da saída escolhe exatamente um pixel da
 * entrada. O pixel (x, y) da saída recebe o pixel (x0 + x, y0 + y).
 * O chamador já conferiu que o retângulo cabe na entrada. */
static void crop(const Image *in, size_t x0, size_t y0, Image *out)
{
    for (size_t y = 0; y < out->h; ++y) {
        for (size_t x = 0; x < out->w; ++x) {
            memcpy(out->pixels + pixel_offset(out, x, y),
                   in->pixels + pixel_offset(in, x0 + x, y0 + y), 3);
        }
    }
}

/* Rotação de 90 graus no sentido horário. A saída tem largura h e altura
 * w. O pixel (x, y) da saída recebe o pixel (y, h - 1 - x) da entrada:
 * a última linha da entrada vira a primeira coluna da saída. */
static void rotate(const Image *in, Image *out)
{
    for (size_t y = 0; y < out->h; ++y) {
        for (size_t x = 0; x < out->w; ++x) {
            memcpy(out->pixels + pixel_offset(out, x, y),
                   in->pixels + pixel_offset(in, y, in->h - 1 - x), 3);
        }
    }
}

int main(int argc, char **argv)
{
    ignore_sigpipe();

    /* Os argumentos são validados antes de ler a imagem. */
    const int rotating = argc == 2 && strcmp(argv[1], "rotate") == 0;
    size_t x0 = 0;
    size_t y0 = 0;
    size_t width = 0;
    size_t height = 0;

    if (!rotating) {
        if (argc != 6 || strcmp(argv[1], "crop") != 0 ||
            !parse_number(argv[2], &x0) || !parse_number(argv[3], &y0) ||
            !parse_number(argv[4], &width) || !parse_number(argv[5], &height)) {
            return error("uso: oxide-image-transform crop X Y W H | rotate");
        }
        if (width == 0 || height == 0) {
            return error("W e H precisam ser positivos");
        }
    }

    Image in;
    Image out = {0, 0, NULL};
    int status = EXIT_FAILURE;

    if (!ppm_read(stdin, &in)) {
        return error("imagem PPM inválida ou erro de leitura");
    }

    if (rotating) {
        out.w = in.h;
        out.h = in.w;
    } else {
        /* As subtrações evitam overflow: x0 + width poderia passar de
         * SIZE_MAX em uma implementação sem o limite de 256. */
        if (x0 > in.w || y0 > in.h || width > in.w - x0 || height > in.h - y0) {
            error("recorte fora da imagem");
            goto cleanup;
        }
        out.w = width;
        out.h = height;
    }

    out.pixels = malloc(out.w * out.h * 3);
    if (out.pixels == NULL) {
        error("falha de alocação");
        goto cleanup;
    }

    if (rotating) {
        rotate(&in, &out);
    } else {
        crop(&in, x0, y0, &out);
    }

    if (!ppm_write(stdout, &out)) {
        error("erro de escrita");
        goto cleanup;
    }
    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (fflush(stdout) == EOF) {
        error("erro de escrita");
        goto cleanup;
    }
    status = EXIT_SUCCESS;

cleanup:
    free(in.pixels);
    free(out.pixels);
    return status;
}
