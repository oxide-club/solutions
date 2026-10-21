/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ppm.h é fornecido pelo desafio em materials/. O Makefile aponta o
 * diretório com -I. Neste desafio, só Image e ppm_write são usados. */
#include "ppm.h"

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

/* Limites do enunciado. */
#define CANVAS_LIMIT 256
#define TRIANGLE_LIMIT 1000
#define COORDINATE_LIMIT 1024
#define CHANNEL_LIMIT 255

/* Um vértice: posição inteira e cor RGB. int64_t evita conversões nos
 * produtos da função de aresta. */
typedef struct {
    int64_t x;
    int64_t y;
    int64_t color[3];
} Vertex;

static void error(const char *message)
{
    fprintf(stderr, "oxide-triangle: %s\n", message);
}

/* Um pipe sem leitor gera SIGPIPE e encerra o processo em silêncio.
 * Com o sinal ignorado, a escrita falha e o programa emite o diagnóstico. */
static void ignore_sigpipe(void)
{
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
}

/* O programa não chama setlocale, então isspace usa a locale "C": espaço,
 * tab, CR, LF, VT e FF, exatamente o whitespace do enunciado. */
static int is_space(int c)
{
    return c != EOF && isspace((unsigned char)c);
}

/* Lê um token inteiro: sinal '-' opcional e um ou mais dígitos.
 * O token termina em whitespace ou EOF. Devolve 0 em qualquer outro texto.
 * A guarda no acumulador rejeita textos longos antes de transbordar;
 * os limites reais são conferidos por read_bounded. */
static int read_integer(int64_t *value)
{
    int c;
    do {
        c = getchar();
    } while (is_space(c));

    const int negative = c == '-';
    if (negative) {
        c = getchar();
    }
    if (c < '0' || c > '9') {
        return 0;
    }

    int64_t magnitude = 0;
    do {
        if (magnitude > 1000000) {
            return 0;
        }
        magnitude = magnitude * 10 + (c - '0');
        c = getchar();
    } while (c >= '0' && c <= '9');

    if (c != EOF && !is_space(c)) {
        return 0;
    }
    *value = negative ? -magnitude : magnitude;
    return 1;
}

static int read_bounded(int64_t *value, int64_t minimum, int64_t maximum)
{
    return read_integer(value) && *value >= minimum && *value <= maximum;
}

/* Função de aresta do enunciado, em coordenadas dobradas:
 *   E(a, b, p) = (bx − ax)(py − ay) − (by − ay)(px − ax)
 * px2 e py2 já vêm dobrados. Os vértices são dobrados aqui. O resultado é
 * 2·E, o que mantém tudo em inteiros. Como área e pesos ficam dobrados na
 * mesma proporção, as razões não mudam. */
static int64_t edge(Vertex a, Vertex b, int64_t px2, int64_t py2)
{
    return (b.x - a.x) * (py2 - 2 * a.y) - (b.y - a.y) * (px2 - 2 * a.x);
}

/* Regra de empate das bordas. Com E > 0, o ponto está do lado interno.
 * Com E = 0, o ponto está sobre a aresta e só uma das duas orientações
 * o aceita. Assim, duas faces que compartilham uma aresta nunca pintam o
 * mesmo pixel, e nenhum pixel sobre a aresta fica sem dono. */
static int accepts(int64_t e, Vertex from, Vertex to)
{
    if (e > 0) {
        return 1;
    }
    return e == 0 && (to.y < from.y || (to.y == from.y && to.x > from.x));
}

/* Pinta um triângulo no canvas. v já tem orientação positiva (area > 0),
 * e area é o valor dobrado devolvido por edge. */
static void rasterize(Image *canvas, const Vertex v[3], int64_t area)
{
    for (size_t y = 0; y < canvas->h; ++y) {
        for (size_t x = 0; x < canvas->w; ++x) {
            /* Centro do pixel: (x + 0.5, y + 0.5), dobrado: (2x+1, 2y+1). */
            const int64_t px2 = 2 * (int64_t)x + 1;
            const int64_t py2 = 2 * (int64_t)y + 1;
            const int64_t e[3] = {
                edge(v[1], v[2], px2, py2),
                edge(v[2], v[0], px2, py2),
                edge(v[0], v[1], px2, py2),
            };
            if (!accepts(e[0], v[1], v[2]) || !accepts(e[1], v[2], v[0]) ||
                !accepts(e[2], v[0], v[1])) {
                continue;
            }

            /* e[i] / area é o peso do vértice i. A divisão inteira
             * (2·soma + area) / (2·area) calcula floor(soma/area + 0.5):
             * arredondamento ao mais próximo, com empate para cima. */
            unsigned char *pixel = canvas->pixels + 3 * (y * canvas->w + x);
            for (int k = 0; k < 3; ++k) {
                const int64_t sum = e[0] * v[0].color[k] +
                                    e[1] * v[1].color[k] + e[2] * v[2].color[k];
                pixel[k] = (unsigned char)((2 * sum + area) / (2 * area));
            }
        }
    }
}

/* Lê os quinze inteiros de um triângulo e confere os limites. */
static int read_triangle(Vertex v[3])
{
    for (int i = 0; i < 3; ++i) {
        if (!read_bounded(&v[i].x, -COORDINATE_LIMIT, COORDINATE_LIMIT) ||
            !read_bounded(&v[i].y, -COORDINATE_LIMIT, COORDINATE_LIMIT)) {
            return 0;
        }
        for (int k = 0; k < 3; ++k) {
            if (!read_bounded(&v[i].color[k], 0, CHANNEL_LIMIT)) {
                return 0;
            }
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        error("uso: oxide-triangle < triangulos.txt > imagem.ppm");
        return EXIT_FAILURE;
    }
    ignore_sigpipe();

    int64_t width;
    int64_t height;
    int64_t count;
    if (!read_bounded(&width, 1, CANVAS_LIMIT) ||
        !read_bounded(&height, 1, CANVAS_LIMIT) ||
        !read_bounded(&count, 0, TRIANGLE_LIMIT)) {
        error("cabeçalho W H N inválido");
        return EXIT_FAILURE;
    }

    /* calloc zera o raster: o canvas começa preto. */
    Image canvas = {(size_t)width, (size_t)height, NULL};
    canvas.pixels = calloc(canvas.w * canvas.h, 3);
    if (canvas.pixels == NULL) {
        error("falha de alocação");
        return EXIT_FAILURE;
    }

    int status = EXIT_FAILURE;
    for (int64_t t = 0; t < count; ++t) {
        Vertex v[3];
        if (!read_triangle(v)) {
            error("triângulo inválido ou fora dos limites");
            goto cleanup;
        }

        /* Área dobrada. Zero: triângulo degenerado, sem pixels. Negativa:
         * orientação invertida. A troca de b e c leva as cores junto, então
         * cada vértice mantém a sua cor. */
        int64_t area = edge(v[0], v[1], 2 * v[2].x, 2 * v[2].y);
        if (area == 0) {
            continue;
        }
        if (area < 0) {
            const Vertex swapped = v[1];
            v[1] = v[2];
            v[2] = swapped;
            area = -area;
        }
        rasterize(&canvas, v, area);
    }

    /* Depois do último triângulo, só whitespace pode restar. */
    int c;
    do {
        c = getchar();
    } while (is_space(c));
    if (ferror(stdin)) {
        error("erro de leitura");
        goto cleanup;
    }
    if (c != EOF) {
        error("tokens extras após o último triângulo");
        goto cleanup;
    }

    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (!ppm_write(stdout, &canvas) || fflush(stdout) == EOF) {
        error("erro de escrita");
        goto cleanup;
    }
    status = EXIT_SUCCESS;

cleanup:
    free(canvas.pixels);
    return status;
}
