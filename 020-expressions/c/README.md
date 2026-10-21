# 020-expressions — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`020-expressions/` no repositório separado `challenges`.

Neste diretório (`solutions/020-expressions/c`), com compilador C11:

```sh
make
printf '2 + 3 * 4' | ./build/oxide-expressions
# 14
printf '(2 + 3) * 4' | ./build/oxide-expressions
# 20
printf '9-3-2' | ./build/oxide-expressions
# 4
printf '(2+3' | ./build/oxide-expressions
# Diagnóstico de parêntese sem fechamento em stderr; status diferente de zero.
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Estratégia de parsing

O programa usa descida recursiva. Cada regra da gramática vira uma
função, e cada função devolve o valor calculado do trecho reconhecido.
Não há árvore sintática: a avaliação acontece durante o parsing.

| Regra | Função | Responsabilidade |
|---|---|---|
| `expr = term { ("+" \| "-") term }` | `parse_expr` | Soma e subtração, associativas à esquerda. |
| `term = atom { "*" atom }` | `parse_term` | Multiplicação, associativa à esquerda. |
| `atom = inteiro \| "(" expr ")"` | `parse_atom` | Literal ou grupo entre parênteses. |

### Precedência

`parse_expr` pede cada operando a `parse_term`, e `parse_term` consome
todas as multiplicações antes de devolver. Em `2 + 3 * 4`, o segundo
operando da soma é o term `3 * 4` inteiro. A multiplicação fica mais
funda na gramática e, por isso, é resolvida primeiro.

### Associatividade

Dentro de uma função, a sequência de operadores do mesmo nível é um laço,
não uma recursão. O laço acumula da esquerda para a direita: `9-3-2` vira
`(9-3)-2`. A recursão só acontece em `parse_atom`, para um parêntese
aberto.

## Como funciona

1. `main` lê toda a entrada em um buffer de 65537 bytes. O byte extra
   detecta entrada acima do limite.
2. `main` chama `parse_expr`. O cursor global avança conforme as funções
   consomem bytes. `skip_spaces` pula separadores antes de cada token.
3. `parse_atom` reconhece um literal, com acumulador int64 conferido a
   cada dígito, ou um grupo. Um grupo incrementa a profundidade, chama
   `parse_expr` e exige `)`.
4. `apply` calcula cada operação em int64 e confere o intervalo de int32.
5. Depois de `parse_expr`, só espaços podem restar. Qualquer outro byte é
   um token excedente.
6. `main` escreve o resultado com LF e confere `fflush`.

## Limites adotados

- Literais vão de 0 a 2147483647. Não há sinal: `-1` é inválido e `0-1`
  é válido.
- Cada resultado intermediário cabe em int32. Overflow é erro, mesmo que
  uma operação posterior voltasse ao intervalo.
- No máximo 64 parênteses abertos ao mesmo tempo. O contador `depth` sobe
  em `(` e desce em `)`. Esse limite também limita a pilha de chamadas,
  porque a recursão só acontece por parênteses.
- Entrada vazia, operando ausente, caractere inválido e byte zero falham
  em `parse_atom`.

## Decisões

`fail` escreve o diagnóstico e chama `exit`. O parser é recursivo, e
devolver um status por todos os níveis tornaria cada função verbosa.
Como nenhuma saída é escrita antes do fim do parsing, encerrar no ponto do
erro não deixa estado a desfazer.

O cursor e a profundidade são variáveis globais. O programa só processa
uma expressão por execução, e as funções ficam mais curtas sem um
parâmetro de contexto.

## Limites

O tempo é O(n) para n bytes: cada byte é consumido uma vez. A pilha de
chamadas tem no máximo 64 níveis de grupo. O programa não aceita
operadores unários, divisão, variáveis ou funções. Uma falha de escrita
pode deixar saída parcial.
