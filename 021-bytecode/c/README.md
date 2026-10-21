# 021-bytecode — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição, os programas de exemplo e os
testes públicos ficam em `021-bytecode/` no repositório separado
`challenges`.

Neste diretório (`solutions/021-bytecode/c`), com compilador C11 e o
checkout de `challenges` ao lado:

```sh
make
printf '\001\003\000\000\000\001\004\000\000\000\002\000' | ./build/oxide-bytecode
# 7
./build/oxide-bytecode < ../../../challenges/021-bytecode/materials/sum-loop.bin
# 6
printf '\010\001\000' | ./build/oxide-bytecode
# Diagnóstico de bytecode inválido em stderr: o salto aponta para um operando.
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

O programa separa a validação da execução.

1. `main` lê o bytecode inteiro em um buffer de 65537 bytes. O byte extra
   detecta entrada acima do limite.
2. `validate` faz a primeira passagem: percorre as instruções em
   sequência, marca cada início no mapa `boundary` e rejeita opcode
   desconhecido, operando incompleto e endereço de memória acima de 15.
3. `validate` faz a segunda passagem: confere que cada alvo de `JMP` e
   `JZ` está dentro do arquivo e é início de instrução. Isso vale para
   todo o código, inclusive o inalcançável.
4. `execute` roda o ciclo buscar-decodificar-executar. Cada iteração
   confere o orçamento, busca o opcode em `pc`, calcula `next` como o
   sucessor e executa. Saltos substituem `next`.
5. `HALT` exige exatamente um valor na pilha, escreve esse valor com LF e
   encerra. `main` confere `fflush`.

## Por que duas passagens

Um salto para a frente aponta para uma posição que a primeira passagem
ainda não visitou. Só depois de marcar todos os inícios é possível saber
se o alvo é um opcode ou o meio de um operando. Por isso, os alvos são
conferidos em uma segunda passagem, com o mapa completo.

## Trace do laço

O programa `materials/sum-loop.bin` soma 3 + 2 + 1 com dois slots de
memória: `mem[0]` é o contador e `mem[1]` é o acumulador. Decodificado:

```text
PC  instrução   efeito
 0  PUSH 3      contador inicial
 5  STORE 0     mem[0] = 3
 7  LOAD 0      topo do laço: empilha o contador
 9  PUSH 0
14  SUB         contador - 0
15  JZ 38       contador zero: sai do laço
18  LOAD 1
20  LOAD 0
22  ADD
23  STORE 1     mem[1] += mem[0]
25  LOAD 0
27  PUSH 1
32  SUB
33  STORE 0     mem[0] -= 1
35  JMP 7       volta ao topo
38  LOAD 1      resultado
40  HALT
```

Primeira iteração, com PC antes da instrução e pilha depois:

| PC | instrução | pilha | mem[0], mem[1] |
|---|---|---|---|
| 0 | PUSH 3 | [3] | 0, 0 |
| 5 | STORE 0 | [] | 3, 0 |
| 7 | LOAD 0 | [3] | 3, 0 |
| 9 | PUSH 0 | [3, 0] | 3, 0 |
| 14 | SUB | [3] | 3, 0 |
| 15 | JZ 38 | [] | 3, 0 |
| 18 | LOAD 1 | [0] | 3, 0 |
| 20 | LOAD 0 | [0, 3] | 3, 0 |
| 22 | ADD | [3] | 3, 0 |
| 23 | STORE 1 | [] | 3, 3 |
| 25 | LOAD 0 | [3] | 3, 3 |
| 27 | PUSH 1 | [3, 1] | 3, 3 |
| 32 | SUB | [2] | 3, 3 |
| 33 | STORE 0 | [] | 2, 3 |
| 35 | JMP 7 | [] | 2, 3 |

O laço repete com o contador em 2 e em 1. Na quarta passagem pelo topo, a
subtração deixa 0 e `JZ` salta para 38. `LOAD 1` empilha 6 e `HALT`
escreve `6`. O trace completo está em `materials/trace.md`.

## Invariantes e limites

- `0 <= top <= 64`. Cada instrução confere a pilha antes de retirar ou
  empilhar.
- `pc` é sempre início de instrução ou o fim do código. Os alvos de salto
  já foram validados. Cair no fim do código é rejeitado na busca
  seguinte.
- Cada busca consome uma unidade do orçamento de 1000, inclusive `HALT`.
  Um programa que encerra na 1000ª instrução é aceito; a 1001ª busca é
  rejeitada.
- Aritmética em int64 com verificação de int32, como nos desafios 019 e
  020.

Custos: a validação é O(n) por passagem, para n bytes de bytecode. O mapa
de fronteiras usa um byte por posição, O(n). A execução é limitada a 1000
instruções, com pilha e memória de tamanho fixo.

## Decisões

`instruction_size` é a única tabela de tamanhos. As duas passagens da
validação e a execução a consultam, então os três lugares concordam.

`i32le` monta o literal como `uint32_t` e converte para int32 por
subtração de 2³². Essa forma não depende de conversão definida pela
implementação para valores negativos.

Um `switch` sobre o opcode mantém cada instrução em um bloco curto. O
caso `default` é inalcançável depois da validação, mas devolve erro em
vez de supor o invariante.

## Limites

O programa não implementa disassembler, trace em `stdout` nem outros
opcodes. Uma falha de escrita pode deixar saída parcial.
