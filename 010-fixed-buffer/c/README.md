# 010-fixed-buffer — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [implementation.c](implementation.c).
Recomendamos tentar o desafio antes de consultá-la. A definição, os
materiais e os testes públicos ficam em `010-fixed-buffer/` no repositório
separado `challenges`.

Os arquivos [api.h](api.h) e [adapter.c](adapter.c) são cópias exatas dos
materiais do desafio. Não os edite: `tools/verify_local.py` confere que as
cópias continuam idênticas. Por isso, eles mantêm a formatação original.

Neste diretório (`solutions/010-fixed-buffer/c`), com compilador C11:

```sh
make
printf '4\na 0001\na ff00\ns\na 01\ns\nc\ns\n' | ./build/oxide-fixed-buffer
# ok
# ok
# 4/4:0001ff00
# fail
# 4/4:0001ff00
# 0/4:
make check
```

`make test` compila também `tests/api_test.c` do desafio contra a
implementação e executa esse teste direto antes dos vetores públicos.
Durante alterações conjuntas dos repositórios, use `make test` para
executar os testes locais sem exigir a revisão fixada no manifesto.
Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

O adaptador lê a capacidade, reserva uma região de até 65536 bytes na
pilha e chama `buffer_init`. Depois converte cada comando em uma chamada
da API e imprime o resultado.

- `buffer_init` guarda o ponteiro da região e a capacidade. O tamanho
  começa em zero.
- `buffer_append` confere se `n` cabe em `capacity - size`. Se cabe,
  copia os bytes para `data + size` e avança `size`. Se não cabe, devolve
  0 e não toca em nada.
- `buffer_clear` zera `size`. A capacidade e a região continuam as mesmas.

## Tamanho e capacidade

`capacity` é o espaço total da região recebida. `size` é a quantidade de
bytes em uso. A invariante `0 <= size <= capacity` vale depois de cada
operação. O espaço livre é `capacity - size`.

## Por que a falha preserva o estado

A verificação acontece antes de qualquer escrita. Quando `n` é maior que o
espaço livre, a função devolve 0 sem chamar `memcpy` e sem alterar `size`.
Os bytes anteriores, o tamanho e a capacidade ficam como estavam. O teste
direto `api_test.c` confirma também que nenhum byte fora da região muda.

## Decisões

A comparação usa `n > capacity - size`, e não `size + n > capacity`. A
subtração nunca transborda porque `size <= capacity`. A soma poderia passar
de `SIZE_MAX` quando `n` é muito grande, como no comando `o`, e parecer
pequena.

Com `n = 0`, a origem pode ser `NULL`. A função não chama `memcpy` nesse
caso, porque passar `NULL` a `memcpy` tem comportamento indefinido, mesmo
com comprimento zero.

## Limites

Cada operação custa O(n) para a cópia ou O(1). A API não usa alocação
dinâmica. Não há truncamento nem consulta de ocupação máxima; o enunciado
lista essas operações como extensões.
