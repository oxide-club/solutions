/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

#define INPUT_LIMIT 65536
#define STACK_LIMIT 64
#define MEMORY_SIZE 16
#define STEP_LIMIT 1000

/* Opcodes do enunciado. O valor numérico é o byte no bytecode. */
enum {
    OP_HALT = 0x00,
    OP_PUSH = 0x01,
    OP_ADD = 0x02,
    OP_SUB = 0x03,
    OP_MUL = 0x04,
    OP_DUP = 0x05,
    OP_LOAD = 0x06,
    OP_STORE = 0x07,
    OP_JMP = 0x08,
    OP_JZ = 0x09,
};

/* O bytecode inteiro fica na memória: a validação precisa ver o programa
 * completo antes da execução. O byte extra detecta entrada acima do
 * limite. */
static unsigned char code[INPUT_LIMIT + 1];
static size_t length;

/* boundary[p] vale 1 quando o byte p é o início de uma instrução. Os
 * saltos só podem apontar para essas posições. */
static unsigned char boundary[INPUT_LIMIT + 1];

static int error(const char *message)
{
    fprintf(stderr, "oxide-bytecode: %s\n", message);
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

/* Tamanho total da instrução, com opcode e operando. Devolve 0 para um
 * opcode desconhecido. */
static size_t instruction_size(unsigned char op)
{
    switch (op) {
    case OP_PUSH:
        return 5;
    case OP_LOAD:
    case OP_STORE:
        return 2;
    case OP_JMP:
    case OP_JZ:
        return 3;
    case OP_HALT:
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DUP:
        return 1;
    default:
        return 0;
    }
}

/* Operandos little endian: o primeiro byte é o menos significativo. */
static unsigned int u16le(size_t offset)
{
    return code[offset] | ((unsigned int)code[offset + 1] << 8);
}

/* int32 em complemento de dois. Os quatro bytes formam um uint32; valores
 * acima de INT32_MAX representam negativos e recebem -2^32. */
static int32_t i32le(size_t offset)
{
    const uint32_t raw =
        (uint32_t)code[offset] | ((uint32_t)code[offset + 1] << 8) |
        ((uint32_t)code[offset + 2] << 16) | ((uint32_t)code[offset + 3] << 24);
    int64_t value = raw;
    if (raw > INT32_MAX) {
        value -= INT64_C(4294967296);
    }
    return (int32_t)value;
}

/* Validação prévia em duas passagens. Devolve 1 se o programa inteiro é
 * válido, inclusive o código inalcançável.
 *
 * Passagem 1: percorre as instruções em sequência. Marca cada início em
 * boundary, rejeita opcode desconhecido, operando incompleto e endereço
 * de memória fora de 0..15.
 *
 * Passagem 2: com todos os inícios marcados, confere cada alvo de salto.
 * Um alvo precisa estar dentro do arquivo e ser início de instrução. A
 * segunda passagem é necessária porque um salto para a frente aponta para
 * uma posição que a primeira passagem ainda não marcou. */
static int validate(void)
{
    for (size_t p = 0; p < length;) {
        const size_t size = instruction_size(code[p]);
        if (size == 0 || size > length - p) {
            return 0;
        }
        boundary[p] = 1;
        if ((code[p] == OP_LOAD || code[p] == OP_STORE) &&
            code[p + 1] >= MEMORY_SIZE) {
            return 0;
        }
        p += size;
    }

    for (size_t p = 0; p < length; p += instruction_size(code[p])) {
        if (code[p] == OP_JMP || code[p] == OP_JZ) {
            const unsigned int target = u16le(p + 1);
            if (target >= length || !boundary[target]) {
                return 0;
            }
        }
    }
    return 1;
}

/* Aplica a + b, a - b ou a * b em int64 e confere o intervalo de int32.
 * int64 comporta o resultado de quaisquer dois int32. */
static int arithmetic(int32_t a, int32_t b, unsigned char op, int32_t *result)
{
    int64_t value;

    if (op == OP_ADD) {
        value = (int64_t)a + b;
    } else if (op == OP_SUB) {
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

/* Ciclo buscar-decodificar-executar. O programa já foi validado, então
 * os operandos existem e os alvos de salto são inícios de instrução.
 *
 * Invariantes: 0 <= top <= STACK_LIMIT; pc é início de instrução ou o fim
 * do código. Cair no fim do código é rejeitado na busca seguinte. */
static int execute(void)
{
    int32_t stack[STACK_LIMIT];
    int32_t memory[MEMORY_SIZE] = {0};
    size_t top = 0;
    size_t pc = 0;

    /* Cada busca consome uma unidade do orçamento, inclusive HALT. */
    for (unsigned int step = 0; step < STEP_LIMIT; ++step) {
        if (pc >= length) {
            return error("execução caiu fora do código");
        }
        const unsigned char op = code[pc];
        size_t next = pc + instruction_size(op);

        switch (op) {
        case OP_HALT:
            if (top != 1) {
                return error("HALT sem exatamente um valor na pilha");
            }
            printf("%" PRId32 "\n", stack[0]);
            return EXIT_SUCCESS;

        case OP_PUSH:
            if (top == STACK_LIMIT) {
                return error("pilha cheia");
            }
            stack[top++] = i32le(pc + 1);
            break;

        case OP_ADD:
        case OP_SUB:
        case OP_MUL: {
            /* Retira b, o operando direito, e depois a. O resultado
             * ocupa o lugar de a. */
            if (top < 2) {
                return error("operação sem dois operandos");
            }
            const int32_t b = stack[--top];
            const int32_t a = stack[top - 1];
            if (!arithmetic(a, b, op, &stack[top - 1])) {
                return error("resultado fora de int32");
            }
            break;
        }

        case OP_DUP:
            if (top == 0 || top == STACK_LIMIT) {
                return error("DUP com pilha vazia ou cheia");
            }
            stack[top] = stack[top - 1];
            ++top;
            break;

        case OP_LOAD:
            if (top == STACK_LIMIT) {
                return error("pilha cheia");
            }
            stack[top++] = memory[code[pc + 1]];
            break;

        case OP_STORE:
            if (top == 0) {
                return error("STORE com pilha vazia");
            }
            memory[code[pc + 1]] = stack[--top];
            break;

        case OP_JMP:
            next = u16le(pc + 1);
            break;

        case OP_JZ:
            if (top == 0) {
                return error("JZ com pilha vazia");
            }
            if (stack[--top] == 0) {
                next = u16le(pc + 1);
            }
            break;

        default:
            /* Inalcançável: validate já rejeitou opcodes desconhecidos. */
            return error("opcode desconhecido");
        }
        pc = next;
    }
    return error("orçamento de 1000 instruções esgotado");
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        return error("uso: oxide-bytecode < programa.bin");
    }
    ignore_sigpipe();

    length = fread(code, 1, sizeof code, stdin);
    if (ferror(stdin)) {
        return error("erro de leitura");
    }
    if (length > INPUT_LIMIT) {
        return error("entrada maior que 65536 bytes");
    }

    if (!validate()) {
        return error("bytecode inválido: opcode, operando, endereço ou salto");
    }

    const int status = execute();

    /* stdio pode guardar a saída no buffer e só falhar no flush final. */
    if (status == EXIT_SUCCESS && (ferror(stdout) || fflush(stdout) == EOF)) {
        return error("erro de escrita");
    }
    return status;
}
