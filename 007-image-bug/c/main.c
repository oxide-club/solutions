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

static int error(const char *message)
{
    fprintf(stderr, "oxide-image-bug: %s\n", message);
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

/* Offset do primeiro canal do pixel (x, y) no raster.
 *
 * O raster guarda as linhas uma após a outra. A linha y começa depois de
 * y linhas completas, e cada linha tem w pixels. Esse passo entre linhas
 * é o stride. O starter usava a altura como stride; a diferença só aparece
 * quando w != h. */
static size_t pixel_offset(const Image *image, size_t x, size_t y)
{
    return (y * image->w + x) * 3;
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        return error("uso: oxide-image-bug < entrada.ppm > saida.ppm");
    }
    ignore_sigpipe();

    Image in;
    Image out = {0, 0, NULL};
    int status = EXIT_FAILURE;

    if (!ppm_read(stdin, &in)) {
        return error("imagem PPM inválida ou erro de leitura");
    }

    /* A saída tem as mesmas dimensões. Só o raster é novo. */
    out.w = in.w;
    out.h = in.h;
    out.pixels = malloc(out.w * out.h * 3);
    if (out.pixels == NULL) {
        error("falha de alocação");
        goto cleanup;
    }

    /* A cópia por coordenadas é intencional: o desafio pede para preservar
     * esse percurso e corrigir apenas o cálculo das posições. */
    for (size_t y = 0; y < in.h; ++y) {
        for (size_t x = 0; x < in.w; ++x) {
            memcpy(out.pixels + pixel_offset(&out, x, y),
                   in.pixels + pixel_offset(&in, x, y), 3);
        }
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
