/* SPDX-License-Identifier: MIT */
#ifndef OXIDE_H
#define OXIDE_H

#include <stddef.h>
#include <stdint.h>

/* Handle opaco: somente Rust aloca e libera sua representação. */
typedef struct OcContext OcContext;

typedef struct {
    int32_t status;
    size_t written;
    size_t required;
} OcResult;

/* Status: 0 Ok; 1 InvalidArgument; 2 BufferTooSmall; 3 InternalPanic;
 * 4 Poisoned. Somente BufferTooSmall informa required no erro. */

/* out == NULL retorna InvalidArgument. Caso contrário, out aponta para um
 * slot vivo, alinhado e gravável de OcContext*. A função inicia *out = NULL
 * e publica um handle somente no sucesso. O cliente passa esse handle para
 * oc_destroy uma única vez; não usa free nem acessa sua representação. */
int32_t oc_create(uint8_t key, OcContext **out);

/* NULL não faz nada. Um handle não nulo deve estar vivo, ter origem em
 * oc_create e não estar em uso. A chamada consome ownership do handle,
 * inclusive após poison. Uso posterior e segunda destruição são inválidos. */
void oc_destroy(OcContext *ctx);

/* ctx == NULL retorna InvalidArgument. Um handle não nulo deve estar vivo;
 * chamadas que o usam devem ser serializadas, inclusive a destruição.
 * src só pode ser NULL com src_len == 0; dst, com dst_cap == 0.
 * Comprimentos maiores que PTRDIFF_MAX são rejeitados antes de acessar os
 * buffers ou consultar poison. Para escalares aceitos, src aponta para
 * src_len bytes vivos, legíveis e inicializados; dst, para dst_cap bytes
 * vivos e graváveis. Regiões não vazias não se sobrepõem entre si nem com
 * o contexto. O cliente preserva ownership dos buffers e seu lifetime até
 * o retorno; a função não retém referências.
 *
 * Validação escalar precede poison, que precede capacidade. BufferTooSmall
 * não escreve, informa written = 0 e required = src_len. No sucesso ambos
 * os comprimentos valem src_len, sem alterar o restante do destino.
 * InternalPanic envenena o contexto; o destino pode estar parcial e deve
 * ser ignorado. Outros erros não escrevem e retornam comprimentos zero.
 * A API não verifica handles falsos, regiões inválidas ou sobreposição.
 * O perfil usa panic=unwind; abort, OOM abortante, UB e sinais não são
 * convertidos em status. A transformação didática não é criptografia. */
OcResult oc_transform(OcContext *ctx, const uint8_t *src, size_t src_len,
                      uint8_t *dst, size_t dst_cap);

/* Helper C: rotação de um bit à esquerda; não lança exceções nem longjmp. */
uint8_t oc_rotate(uint8_t value);

#ifdef OC_TEST_HOOKS
/* Somente no build de testes; arma panic na próxima transformação elegível. */
void oc_test_panic_next(void);
#endif

#endif
