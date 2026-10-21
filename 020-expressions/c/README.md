# Referência C11 — Interpretador de expressões

`make` compila; `make test` usa o checkout local; `make check` também exige a revisão fixada; `make clean` remove o binário. Leia o contrato em `../../../challenges/020-expressions/index.md`. O programa limita a entrada a 65536 bytes, valida antes de produzir o resultado e verifica erros de I/O.

## Estratégia e invariantes

atom lê literal ou grupo, term resolve multiplicações, expr resolve somas/subtrações à esquerda. A recursão ocorre só por parênteses, limitada a 64; sequências de operadores são laços. Literais e intermediários usam int64 para verificar int32 antes da conversão. Tempo O(n), pilha de chamadas O(64).
