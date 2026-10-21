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
#define STACK_LIMIT 64

/* A entrada inteira fica na memória. O byte extra detecta entrada acima
 * do limite. O buffer não é uma string: o byte zero é um token inválido,
 * não um terminador. */
static unsigned char data[INPUT_LIMIT + 1];
static size_t length;

static int error(const char *message)
{
    fprintf(stderr, "oxide-rpn: %s\n", message);
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

/* O programa não chama setlocale, então isspace usa a locale "C": espaço,
 * tab, CR, LF, VT e FF, exatamente os separadores do enunciado. */
static int is_space(unsigned char c)
{
    return isspace(c);
}

/* Aplica a + b, a - b ou a * b. Devolve 0 se o resultado não cabe em
 * int32. int64 comporta a soma, a diferença e o produto de quaisquer dois
 * int32, então a conta nunca transborda antes da verificação. */
static int apply(int32_t a, int32_t b, unsigned char op, int32_t *result)
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
        return 0;
    }
    *result = (int32_t)value;
    return 1;
}

/* Converte um token em literal int32: sinal '-' opcional e um ou mais
 * dígitos. Devolve 0 para qualquer outro texto ou valor fora do intervalo.
 *
 * O acumulador é int64 e é conferido a cada dígito. O limite aceito é
 * INT32_MAX para positivos e INT32_MAX + 1 para negativos, porque
 * -2147483648 é um int32 válido. */
static int parse_literal(const unsigned char *token, size_t size,
                         int32_t *value)
{
    size_t i = 0;
    const int negative = size > 0 && token[0] == '-';
    if (negative) {
        i = 1;
    }
    if (i == size) {
        return 0;
    }

    int64_t magnitude = 0;
    for (; i < size; ++i) {
        if (token[i] < '0' || token[i] > '9') {
            return 0;
        }
        magnitude = magnitude * 10 + (token[i] - '0');
        if (magnitude > (int64_t)INT32_MAX + negative) {
            return 0;
        }
    }
    *value = (int32_t)(negative ? -magnitude : magnitude);
    return 1;
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        return error("uso: oxide-rpn < expressao.txt");
    }
    ignore_sigpipe();

    length = fread(data, 1, sizeof data, stdin);
    if (ferror(stdin)) {
        return error("erro de leitura");
    }
    if (length > INPUT_LIMIT) {
        return error("entrada maior que 65536 bytes");
    }

    /* Invariante: 0 <= top <= STACK_LIMIT. top é a quantidade de valores
     * na pilha; stack[top - 1] é o topo. */
    int32_t stack[STACK_LIMIT];
    size_t top = 0;
    size_t cursor = 0;

    while (cursor < length) {
        if (is_space(data[cursor])) {
            ++cursor;
            continue;
        }

        /* Um token vai do byte atual até o próximo separador ou o fim. */
        const size_t start = cursor;
        while (cursor < length && !is_space(data[cursor])) {
            ++cursor;
        }
        const unsigned char *token = data + start;
        const size_t size = cursor - start;

        const int is_operator =
            size == 1 &&
            (token[0] == '+' || token[0] == '-' || token[0] == '*');
        if (is_operator) {
            /* O operador retira primeiro b, o operando direito, e depois
             * a. O resultado ocupa o lugar de a. */
            if (top < 2) {
                return error("operador sem dois operandos");
            }
            const int32_t b = stack[--top];
            const int32_t a = stack[top - 1];
            if (!apply(a, b, token[0], &stack[top - 1])) {
                return error("resultado fora de int32");
            }
        } else {
            int32_t value;
            if (!parse_literal(token, size, &value)) {
                return error("token inválido ou literal fora de int32");
            }
            if (top == STACK_LIMIT) {
                return error("pilha cheia: 65º valor");
            }
            stack[top++] = value;
        }
    }

    /* Uma expressão válida deixa exatamente um valor. Entrada vazia deixa
     * zero valores e também é rejeitada aqui. */
    if (top != 1) {
        return error("a expressão não deixa exatamente um valor");
    }
    printf("%" PRId32 "\n", stack[0]);

    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (ferror(stdout) || fflush(stdout) == EOF) {
        return error("erro de escrita");
    }
    return EXIT_SUCCESS;
}
