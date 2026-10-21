# Referência C11 — Uma VM de bytecode

`make` compila; `make test` usa o checkout local; `make check` também exige a revisão fixada; `make clean` remove o binário. Leia o contrato em `../../../challenges/021-bytecode/index.md`. O programa limita a entrada a 65536 bytes, valida antes de produzir o resultado e verifica erros de I/O.

## Estratégia e invariantes

A primeira passagem marca inícios e confere tamanho de cada operando e endereços. A segunda verifica todos os saltos, inclusive inalcançáveis. Na execução, top delimita a pilha, next começa no sucessor e saltos o substituem. Cada busca consome orçamento antes de executar, inclusive HALT. O laço soma 3+2+1 usando dois slots de memória. Tempo O(n+1000), mapa de fronteiras O(n) e memória de execução fixa.
