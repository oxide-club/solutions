# Referência C11 — Inspetor de um formato documentado

`make` compila; `make test` usa o checkout local; `make check` também exige a revisão fixada; `make clean` remove o binário. Leia o contrato em `../../../challenges/013-record-inspector/index.md`. O programa limita a entrada a 65536 bytes, valida antes de produzir o resultado e verifica erros de I/O.

## Estratégia e invariantes

O inspetor faz duas passagens: valida todos os registros e o fim exato antes de imprimir. Em cada acesso exige bytes suficientes e compara comprimento com len-p, evitando soma que possa transbordar. Tempo O(n), memória limitada ao buffer de entrada.
