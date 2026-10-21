# 004-hexdump — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`004-hexdump/` no repositório separado `challenges`.

Neste diretório (`solutions/004-hexdump/c`), com compilador C11:

```sh
make
printf 'Oxide Club\n' | ./build/oxide-hexdump
# 00000000  4f 78 69 64 65 20 43 6c 75 62 0a                 |Oxide Club.|
./build/oxide-hexdump main.c | head -n 2
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

1. `main` valida a quantidade de argumentos e ignora `SIGPIPE`.
2. Sem argumento, a entrada é `stdin`. Com um argumento, `fopen` abre o
   arquivo em modo binário.
3. O laço lê blocos de 16 bytes com `fread`. Cada bloco vira uma linha.
4. `write_line` escreve o offset com oito dígitos, dezesseis campos de três
   caracteres e a coluna ASCII entre barras verticais.
5. O programa soma o tamanho do bloco ao offset e repete até o fim da entrada.
6. No fim, `main` confere `ferror` na entrada e na saída e chama `fflush`.

## Decisões

`fread` com tamanho 16 só devolve um bloco menor no fim da entrada ou em
erro. Por isso, apenas a última linha pode ficar incompleta. As posições
ausentes recebem três espaços. Esse preenchimento mantém a coluna ASCII
alinhada, como o enunciado exige.

A coluna ASCII mostra somente os bytes de `0x20` a `0x7e`. Os demais viram
`.`. A comparação usa os valores numéricos e não depende da locale.

O offset usa `uint64_t`. O programa confere o overflow com uma subtração
antes de somar. Assim, a soma nunca ultrapassa o limite do tipo.

As chamadas de escrita não são verificadas uma a uma. `stdio` registra a
falha no fluxo. No fim, `main` consulta `ferror(stdout)` e o resultado de
`fflush`. Esse padrão mantém `write_line` curta e cobre também o pipe
fechado, porque o programa ignora `SIGPIPE`.

## Limites

O tempo é O(n) para n bytes de entrada. A memória adicional é O(1): um
bloco de 16 bytes e o offset. Uma falha pode deixar saída parcial. O
programa não implementa opções de largura, agrupamento ou cores.
