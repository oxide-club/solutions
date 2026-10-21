# 006-stream-search — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`006-stream-search/` no repositório separado `challenges`.

Neste diretório (`solutions/006-stream-search/c`), com compilador C11:

```sh
make
printf 'abababa' | ./build/oxide-stream-search 616261
# 0
# 2
# 4
printf 'aaaaa' | ./build/oxide-stream-search 616161
# 0
# 1
# 2
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

1. `main` valida o argumento: não vazio, quantidade par de dígitos e no
   máximo 2 097 152 dígitos, que correspondem a 1 MiB de padrão.
2. `parse_pattern` converte cada par de dígitos em um byte.
3. `build_prefix_table` calcula a tabela de prefixos do algoritmo KMP.
4. O laço principal lê blocos de 4096 bytes com `fread`.
5. Para cada byte, o programa compara com `pattern[matched]`. Em uma
   falha, `matched` recua para `prefix[matched - 1]`. Em um acerto,
   `matched` avança.
6. Quando `matched` chega ao tamanho do padrão, o programa escreve o
   offset inicial e recua pela tabela. Esse recuo permite ocorrências
   sobrepostas.
7. `position` conta os bytes lidos e sobrevive entre blocos. No fim, `main`
   confere `ferror` e chama `fflush`.

## Por que o tempo é O(n + m)

A tabela de prefixos custa O(m) para m bytes de padrão. O argumento é o
mesmo usado na busca.

Na busca, considere o valor de `matched`. Cada byte da entrada aumenta
`matched` no máximo em um. Cada iteração do laço de recuo diminui
`matched` em pelo menos um, porque `prefix[matched - 1] < matched`.
`matched` nunca fica negativo. Portanto, o total de recuos ao longo de
toda a entrada é no máximo o total de avanços, que é no máximo n. O laço
interno executa O(n) vezes no total, não O(n) por byte.

O algoritmo nunca relê um byte da entrada. Depois de uma falha, os bytes
já comparados continuam casados com um prefixo menor do padrão. A tabela
registra qual prefixo.

## Decisões

O estado da busca são duas variáveis: `matched` e `position`. Elas ficam
fora do laço de blocos. Assim, uma ocorrência que começa em um bloco e
termina no seguinte é encontrada, como o vetor `search4` exige.

O offset de uma ocorrência é `position + 1 - length`. A soma vem antes da
subtração para evitar um resultado negativo intermediário em `uint64_t`.

`position` é verificada contra `UINT64_MAX` antes de cada incremento. O
programa rejeita overflow com diagnóstico.

A liberação de `pattern` e `prefix` fica em um único ponto, depois do
rótulo `cleanup`. Cada caminho de erro grava o diagnóstico e salta para
esse ponto.

## Limites

A memória adicional é O(m + B): o padrão, a tabela de prefixos e um bloco
de 4096 bytes. A tabela usa `size_t` por posição, então o custo real é
cerca de nove bytes por byte de padrão. Uma falha de escrita pode deixar
offsets parciais na saída.
