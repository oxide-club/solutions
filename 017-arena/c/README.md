# 017-arena — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [implementation.c](implementation.c).
Recomendamos tentar o desafio antes de consultá-la. A definição, os
materiais e os testes públicos ficam em `017-arena/` no repositório
separado `challenges`.

Os arquivos [api.h](api.h) e [adapter.c](adapter.c) são cópias exatas dos
materiais do desafio. Não os edite: `tools/verify_local.py` confere que as
cópias continuam idênticas. Por isso, eles mantêm a formatação original.

Neste diretório (`solutions/017-arena/c`), com compilador C11:

```sh
make
printf '32\na 1 1\na 3 8\ns\na 1 16\na 17 1\ns\nr\na 32 16\ns\n' | ./build/oxide-arena
# 0
# 8
# 11/32
# 16
# fail
# 17/32
# 0
# 32/32
make check
```

`make test` compila também `tests/api_test.c` do desafio contra a
implementação e executa esse teste direto antes dos vetores públicos.
Durante alterações conjuntas dos repositórios, use `make test` para
executar os testes locais sem exigir a revisão fixada no manifesto.
Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

A arena guarda três valores: a base da região, a capacidade e `used`, o
offset do primeiro byte livre.

- `arena_init` guarda a base e a capacidade. `used` começa em zero.
- `arena_alloc` valida `n` e o alinhamento. Calcula o preenchimento até o
  próximo múltiplo do alinhamento. Confere se preenchimento e `n` cabem.
  Avança `used` e devolve `base + offset`.
- `arena_reset` zera `used`. Não existe liberação individual.

## Diagrama de uma sequência

Capacidade 32, com os comandos do exemplo acima. Cada célula é um byte.
`U` é byte útil, `P` é preenchimento e `.` é livre.

```text
a 1 1    offset 0           U...............................   used = 1
a 3 8    offset 8           UPPPPPPPUUU.....................   used = 11
a 1 16   offset 16          UPPPPPPPUUUPPPPPU...............   used = 17
a 17 1   fail               UPPPPPPPUUUPPPPPU...............   used = 17
r                           ................................   used = 0
a 32 16  offset 0           UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU   used = 32
```

O segundo pedido precisa começar em um múltiplo de 8. `used` vale 1, então
o programa pula sete bytes. Esse preenchimento consome capacidade: depois
dos três bytes úteis, `used` vale 11. O pedido de 17 bytes não cabe nos 15
bytes restantes e falha sem mover `used`.

## Invalidação após o reset

`arena_reset` não apaga bytes nem devolve memória ao sistema. Ele só zera
`used`. A próxima alocação pode devolver o mesmo endereço de uma alocação
anterior, como no diagrama: o offset 0 aparece duas vezes. Um ponteiro
guardado antes do reset aponta agora para dados de outra alocação. Por
isso, todos os ponteiros entregues antes do reset ficam inválidos, mesmo
quando o endereço parece o mesmo.

## Decisões

O preenchimento usa a máscara `alignment - 1`, válida porque o alinhamento
é uma potência de dois. A expressão
`(alignment - (used & (alignment - 1))) & (alignment - 1)` devolve zero
quando `used` já está alinhado, sem um `if` separado.

As verificações de espaço usam subtração: `padding > capacity - used` e
`n > capacity - start`. A soma `used + padding + n` poderia passar de
`SIZE_MAX` no comando `o` e parecer pequena. As subtrações nunca
transbordam porque `used <= capacity` e `start <= capacity` valem nesses
pontos.

Tamanho zero é rejeitado, como a API define. Alinhamento zero, não
potência de dois ou acima de 16 também devolve `NULL`.

## Limites

Cada alocação custa O(1). A arena não oferece marcas nem liberação
individual; o enunciado lista as marcas como extensão. O alinhamento
máximo é 16, porque a base só garante esse alinhamento.
