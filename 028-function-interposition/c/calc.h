/* SPDX-License-Identifier: MIT */
#ifndef OX_CALC_H
#define OX_CALC_H
#include <stddef.h>
int calc_div(int a, int b, int *out);
/* Callback exportado pelo cliente. Uma chamada recebe um JSON completo com LF.
   Retorno 0: recebido; -1: perdido. Pode alterar errno ou reentrar calc_div. */
int ox_trace_sink(const char *line, size_t length);
#endif
