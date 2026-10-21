# 016-use-after-free — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [implementation.c](implementation.c).
Recomendamos tentar o desafio antes de consultá-la. A definição, o starter
`materials/buggy.c`, o relatório do sanitizador e os testes públicos ficam
em `016-use-after-free/` no repositório separado `challenges`.

Os arquivos [api.h](api.h) e [adapter.c](adapter.c) são cópias exatas dos
materiais do desafio. Não os edite: `tools/verify_local.py` confere que as
cópias continuam idênticas. Por isso, eles mantêm a formatação original.

Neste diretório (`solutions/016-use-after-free/c`), com compilador C11 e o
checkout de `challenges` ao lado:

```sh
make
printf 'A\000B\377' | ./build/oxide-use-after-free
# 410042ff
make sanitize
python3 ../../../challenges/016-use-after-free/tests/sanitizer_regression.py implementation.c
make check
```

`make sanitize` compila o teste direto e o programa com AddressSanitizer
e UndefinedBehaviorSanitizer e executa os dois. O script
`sanitizer_regression.py` exige o diagnóstico `heap-use-after-free` no
starter e sucesso da implementação corrigida nos mesmos casos. Durante
alterações conjuntas dos repositórios, use `make test` para executar os
testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Linha do tempo do starter

O relatório do sanitizador mostra três pilhas: alocação, liberação e
acesso. Elas reconstroem esta sequência:

1. **Criação.** `produce` chama `malloc(4)` e copia quatro bytes para a
   região.
2. **Liberação prematura.** Ainda dentro de `produce`, `free(p)` encerra o
   tempo de vida da região. `produce` é o responsável por esse `free`.
3. **Devolução de um ponteiro morto.** `produce` devolve o endereço
   antigo. O valor do ponteiro não muda, mas a memória apontada já não
   pertence ao programa.
4. **Acesso inválido.** `main` lê `p[0]`. O sanitizador reporta
   `heap-use-after-free` nessa linha.

O byte que uma execução sem sanitizador imprime é acidental. Um acesso
fora do tempo de vida tem comportamento indefinido, e a regressão não
pode se basear nesse valor.

## Correção

O contrato de `api.h` define quem possui cada região:

- A origem pertence ao chamador. `copy_bytes` só a lê.
- A cópia passa a pertencer ao chamador. Ela continua válida até o
  chamador chamar `free`.

`copy_bytes` aloca, copia e devolve. A função nunca libera. O adaptador
lê a cópia para imprimir os bytes e só depois chama `free`. Assim, o
ponteiro devolvido continua válido em todo o intervalo de uso.

Com `n = 0`, a função aloca um byte. `malloc(0)` pode devolver `NULL` em
algumas implementações, e o contrato exige uma alocação liberável também
nesse caso. Se `malloc` falha, a função devolve `NULL` e o adaptador
reporta erro em `stderr` com status diferente de zero.

## Como funciona

1. O adaptador lê até 65536 bytes brutos de `stdin`, inclusive zeros.
2. `copy_bytes` cria uma cópia independente com o comprimento explícito.
   Não há terminador: o byte `00` é dado.
3. O adaptador imprime dois dígitos hexadecimais por byte e uma quebra de
   linha.
4. O adaptador libera a cópia. A liberação acontece no consumidor, depois
   do último uso.

## Regressão

O teste direto `api_test.c` altera a origem depois da cópia e confere que
a cópia mantém os bytes anteriores. Isso prova a independência. O script
`sanitizer_regression.py` compila o starter e a correção com ASan: o
starter precisa falhar com `heap-use-after-free`; a correção precisa
passar em todos os vetores.

## Limites

A cópia custa O(n) em tempo e memória. A API não oferece falha de
alocação injetável; o enunciado lista essa variante como extensão.
