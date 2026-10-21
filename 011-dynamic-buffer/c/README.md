# 011-dynamic-buffer — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [implementation.c](implementation.c).
Recomendamos tentar o desafio antes de consultá-la. A definição, os
materiais e os testes públicos ficam em `011-dynamic-buffer/` no
repositório separado `challenges`.

Os arquivos [api.h](api.h) e [adapter.c](adapter.c) são cópias exatas dos
materiais do desafio. Não os edite: `tools/verify_local.py` confere que as
cópias continuam idênticas. Por isso, eles mantêm a formatação original.

Neste diretório (`solutions/011-dynamic-buffer/c`), com compilador C11:

```sh
make
printf 'a 00\ns\nf\na 0102030405060708\ns\na 0102030405060708\ns\n' | ./build/oxide-dynamic-buffer
# ok
# 1/8:00
# fail
# 1/8:00
# ok
# 9/16:000102030405060708
make check
```

`make test` compila também `tests/api_test.c` do desafio contra a
implementação e executa esse teste direto antes dos vetores públicos.
Durante alterações conjuntas dos repositórios, use `make test` para
executar os testes locais sem exigir a revisão fixada no manifesto.
Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

O adaptador fornece um callback `resize` que chama `realloc`. O comando
`f` faz a próxima chamada real devolver `NULL`.

- `buffer_init` começa com `data = NULL`, tamanho zero e capacidade zero.
- `buffer_append` confere o overflow de `size + n`. Se o total não cabe,
  `grown_capacity` calcula a nova reserva e o callback realoca. Só depois
  do sucesso o buffer publica o novo ponteiro e a nova capacidade. Então
  copia os bytes e avança `size`.
- `buffer_destroy` libera com `free` e volta ao estado inicial.

## Crescimento geométrico

A primeira reserva tem oito bytes. Cada crescimento dobra a capacidade até
caber o novo conteúdo. Se a duplicação passaria de `SIZE_MAX`, a função
usa exatamente o tamanho necessário.

Dobrar mantém o custo acumulado linear. Para chegar a n bytes, o buffer
copia 8 + 16 + 32 + … + n bytes nas realocações. Essa soma é menor que
2n. Cada byte é copiado em média menos de duas vezes, então o custo
amortizado por append é O(1). Crescer de um valor fixo por vez, como 8
bytes, custaria O(n²) no total.

## Tamanho e capacidade

`size` é a quantidade de bytes em uso. `capacity` é a reserva atual. O
comando `s` imprime os dois: `9/16` significa nove bytes usados em uma
reserva de dezesseis.

## Por que a falha preserva o estado

O callback segue o contrato de `realloc`: em falha, devolve `NULL` e
mantém a região antiga válida. O código guarda o resultado em uma variável
separada e só atribui `data` e `capacity` quando o resultado não é `NULL`.
Uma falha deixa ponteiro, conteúdo, tamanho e capacidade como estavam, e o
append seguinte pode funcionar.

A verificação de overflow vem antes de tudo. O comando `o` pede
`SIZE_MAX` bytes de origem `NULL`: a função devolve 0 sem ler a origem e
sem chamar o alocador.

## Limites

Um append custa O(n) para a cópia, mais o custo amortizado O(1) do
crescimento. O buffer não reduz a capacidade e não oferece reserva mínima
explícita; o enunciado lista essas operações como extensões.
