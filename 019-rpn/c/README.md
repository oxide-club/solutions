# Referência C11 — Calculadora RPN

`make` compila; `make test` usa o checkout local; `make check` também exige a revisão fixada; `make clean` remove o binário. Leia o contrato em `../../../challenges/019-rpn/index.md`. O programa limita a entrada a 65536 bytes, valida antes de produzir o resultado e verifica erros de I/O.

## Estratégia e invariantes

O cursor delimita tokens sem tratar zero como terminador. A pilha mantém top entre 0 e 64. Literais acumulam em int64 e são verificados a cada dígito; operações também usam int64 antes da conversão. Tempo O(n), espaço O(64) além do buffer de entrada.
