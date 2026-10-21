/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

#define INPUT_LIMIT 65536
#define DEPTH_LIMIT 64

/* Gramática reconhecida, com uma função por regra:
 *
 *   expr = term { ("+" | "-") term }    parse_expr
 *   term = atom { "*" atom }            parse_term
 *   atom = inteiro | "(" expr ")"       parse_atom
 *
 * A regra mais externa tem a menor precedência. Multiplicação fica em
 * term, abaixo de expr, então é resolvida antes de soma e subtração. */

/* A entrada inteira fica na memória. O byte extra detecta entrada acima
 * do limite. O buffer não é uma string: o byte zero é um caractere
 * inválido, não um terminador. */
static unsigned char data[INPUT_LIMIT + 1];
static size_t length;

/* Estado do parser: posição do próximo byte e profundidade de parênteses
 * abertos. */
static size_t cursor;
static unsigned int depth;

/* O parser é recursivo. Devolver um status por todos os níveis tornaria
 * cada função verbosa. Como nenhuma saída foi escrita antes do fim do
 * parsing, encerrar no ponto do erro não deixa estado a desfazer. */
static void fail(const char *message)
{
    fprintf(stderr, "oxide-expressions: %s\n", message);
    exit(EXIT_FAILURE);
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
 * tab, CR, LF, VT e FF, exatamente os separadores do enunciado. */
static void skip_spaces(void)
{
    while (cursor < length && isspace(data[cursor])) {
        ++cursor;
    }
}

/* Aplica a + b, a - b ou a * b em int64 e confere o intervalo de int32.
 * int64 comporta o resultado de quaisquer dois int32, então a conta nunca
 * transborda antes da verificação. */
static int32_t apply(int32_t a, int32_t b, unsigned char op)
{
    int64_t value;

    if (op == '+') {
        value = (int64_t)a + b;
    } else if (op == '-') {
        value = (int64_t)a - b;
    } else {
        value = (int64_t)a * b;
    }
    if (value < INT32_MIN || value > INT32_MAX) {
        fail("resultado fora de int32");
    }
    return (int32_t)value;
}

static int32_t parse_expr(void);

/* atom = inteiro | "(" expr ")" */
static int32_t parse_atom(void)
{
    skip_spaces();
    if (cursor == length) {
        fail("operando ausente");
    }

    if (data[cursor] == '(') {
        ++cursor;
        /* A recursão só acontece aqui. O limite de profundidade também
         * limita a pilha de chamadas. */
        if (++depth > DEPTH_LIMIT) {
            fail("mais de 64 parênteses abertos");
        }
        const int32_t value = parse_expr();
        skip_spaces();
        if (cursor == length || data[cursor] != ')') {
            fail("parêntese sem fechamento");
        }
        ++cursor;
        --depth;
        return value;
    }

    /* Literal: um ou mais dígitos, sem sinal. "-1" é inválido; "0-1" é
     * válido. O acumulador int64 é conferido a cada dígito. */
    if (data[cursor] < '0' || data[cursor] > '9') {
        fail("caractere inválido");
    }
    int64_t value = 0;
    while (cursor < length && data[cursor] >= '0' && data[cursor] <= '9') {
        value = value * 10 + (data[cursor] - '0');
        if (value > INT32_MAX) {
            fail("literal fora de int32");
        }
        ++cursor;
    }
    return (int32_t)value;
}

/* term = atom { "*" atom }
 * O laço, e não a recursão, trata a sequência de operadores. Por isso
 * "2*3*4" não aumenta a profundidade e associa à esquerda: (2*3)*4. */
static int32_t parse_term(void)
{
    int32_t value = parse_atom();

    skip_spaces();
    while (cursor < length && data[cursor] == '*') {
        ++cursor;
        value = apply(value, parse_atom(), '*');
        skip_spaces();
    }
    return value;
}

/* expr = term { ("+" | "-") term }
 * Cada operando é um term inteiro, então "2 + 3 * 4" calcula 3 * 4 antes
 * da soma. A sequência associa à esquerda: "9-3-2" é (9-3)-2. */
static int32_t parse_expr(void)
{
    int32_t value = parse_term();

    skip_spaces();
    while (cursor < length && (data[cursor] == '+' || data[cursor] == '-')) {
        const unsigned char op = data[cursor++];
        value = apply(value, parse_term(), op);
        skip_spaces();
    }
    return value;
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        fail("uso: oxide-expressions < expressao.txt");
    }
    ignore_sigpipe();

    length = fread(data, 1, sizeof data, stdin);
    if (ferror(stdin)) {
        fail("erro de leitura");
    }
    if (length > INPUT_LIMIT) {
        fail("entrada maior que 65536 bytes");
    }

    const int32_t value = parse_expr();

    /* Depois da expressão, só espaços podem restar. "1 2" e "(2+3))"
     * param aqui com um token excedente. */
    skip_spaces();
    if (cursor != length) {
        fail("token excedente após a expressão");
    }

    printf("%" PRId32 "\n", value);

    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (ferror(stdout) || fflush(stdout) == EOF) {
        fail("erro de escrita");
    }
    return EXIT_SUCCESS;
}
