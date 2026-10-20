/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Oxide Club contributors
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

_Static_assert(CHAR_BIT == 8, "Este desafio exige bytes de 8 bits");

static int write_error(void)
{
    fputs("oxide-ppm: erro de escrita\n", stderr);
    return EXIT_FAILURE;
}

int main(void)
{
    if (fputs("P6\n256 256\n255\n", stdout) == EOF) {
        return write_error();
    }

    /* A ordem dos laços define a ordem de varredura: cada linha da esquerda
     * para a direita, antes de passar à próxima linha. */
    for (unsigned int y = 0; y < 256; ++y) {
        for (unsigned int x = 0; x < 256; ++x) {
            const unsigned char pixel[3] = {
                (unsigned char)x, (unsigned char)y, 0
            };
            /* P6 usa bytes RGB, não os dígitos ASCII dos valores dos canais.
             * Um único pixel é suficiente como armazenamento temporário. */
            if (fwrite(pixel, 1, sizeof pixel, stdout) != sizeof pixel) {
                return write_error();
            }
        }
    }

    /* stdio pode aceitar dados no buffer e só encontrar o erro ao esvaziá-lo.
     * Verificar apenas fwrite não garante que a saída foi entregue. */
    if (fflush(stdout) == EOF) {
        return write_error();
    }
    return EXIT_SUCCESS;
}
