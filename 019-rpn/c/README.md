# 019-rpn — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`019-rpn/` no repositório separado `challenges`.

Neste diretório (`solutions/019-rpn/c`), com compilador C11:

```sh
make
printf '3 4 + 2 *' | ./build/oxide-rpn
# 14
printf '3 7 -' | ./build/oxide-rpn
# -4
printf '2147483647 1 +' | ./build/oxide-rpn
# Diagnóstico de resultado fora de int32 em stderr; status diferente de zero.
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

1. `main` lê toda a entrada em um buffer de 65537 bytes. O byte extra
   detecta entrada acima do limite.
2. O laço principal pula separadores e delimita o próximo token: do byte
   atual até o próximo separador ou o fim.
3. Um token de um byte igual a `+`, `-` ou `*` é um operador. O programa
   retira `b` e depois `a` da pilha, chama `apply` e grava o resultado no
   lugar de `a`.
4. Qualquer outro token passa por `parse_literal`. Um literal válido é
   empilhado, se a pilha tem espaço.
5. No fim, a pilha precisa ter exatamente um valor. O programa escreve
   esse valor com LF e confere `fflush`.

Para `3 4 + 2 *`, a pilha evolui assim: `[3]`, `[3, 4]`, `[7]`, `[7, 2]`,
`[14]`.

## Invariantes e limites

- `0 <= top <= 64`. `top` é a quantidade de valores. Um operador exige
  `top >= 2`. Um literal exige `top < 64`; o 65º valor é rejeitado.
- Todo valor na pilha cabe em int32. `parse_literal` confere o literal a
  cada dígito, com um acumulador int64. O limite é `INT32_MAX` para
  positivos e `INT32_MAX + 1` para negativos, porque `-2147483648` é
  válido.
- `apply` calcula em int64. A soma, a diferença e o produto de dois int32
  cabem em int64, então a conta nunca transborda antes da verificação. Um
  resultado fora de int32 é erro, mesmo que a operação seguinte voltasse
  ao intervalo.

## Decisões

O buffer de entrada não é tratado como string. O laço usa `length` como
limite e nunca procura um terminador. Por isso, um byte zero dentro de um
token é um byte inválido, não o fim da entrada.

O token é delimitado antes da classificação. `-` sozinho é um operador;
`-5` é um literal; `-` seguido de letra é inválido. A distinção usa o
tamanho do token e o primeiro byte.

A ordem dos operandos segue o enunciado: o operador retira primeiro o
operando direito. `3 7 -` calcula `3 - 7`, e não `7 - 3`.

Entrada vazia e `1 2` falham na mesma verificação final: a pilha não tem
exatamente um valor.

## Limites

O tempo é O(n) para n bytes. A pilha usa 64 valores de int32. O programa
não aceita `+` unário, divisão ou outros operadores. Uma falha de escrita
pode deixar saída parcial.
