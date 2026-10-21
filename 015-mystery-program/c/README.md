# 015-mystery-program — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição, o alvo, a listagem de
assembly e os traces ficam em `015-mystery-program/` no repositório
separado `challenges`.

ATENÇÃO: este README revela a transformação. Faça a sua investigação
antes de continuar.

Neste diretório (`solutions/015-mystery-program/c`), com compilador C11 e
o checkout de `challenges` ao lado:

```sh
make
printf 'ABC' | ./build/oxide-mystery-program | od -An -tx1
# 1b 19 1b
printf '\000\000\000' | ./build/oxide-mystery-program | od -An -tx1
# 5c 5b 5a
printf 'ABC' | ../../../challenges/015-mystery-program/materials/mystery-linux-x86_64 | od -An -tx1
# 1b 19 1b (o alvo é o oráculo; só Linux x86-64)
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Registro da investigação

| Hipótese | Experimento | Observação | Conclusão |
|---|---|---|---|
| Cada byte recebe uma constante. | `00` | `5a` | Um byte zero vira `5a`. A operação pode ser XOR ou soma com `0x5a`. |
| A posição não influencia. | `00 00 00` | `5c 5b 5a` | Refutada. Três zeros produzem valores diferentes: a posição entra no cálculo, e a saída parece invertida. |
| A operação é soma com `0x5a`. | `41 42 43` | `1b 19 1b` | Refutada. `0x41 + 0x5a = 0x9b`. Com XOR, `0x41 ^ 0x5a = 0x1b`, `0x42 ^ 0x5a = 0x18` e `0x43 ^ 0x5a = 0x19`. Somar a posição dá `1b`, `19`, `1b`; invertidos, `1b 19 1b`. |
| A posição somada é a original, não a de saída. | Impulsos: um byte `ff` em posições diferentes de uma entrada de zeros. | O byte alterado muda conforme a posição original `i`, e sai na posição `n − 1 − i`. | Confirmada. |
| O resultado usa aritmética módulo 256. | Bytes altos, como `ff` nas últimas posições de 64 bytes. | O alvo e a implementação coincidem. | Confirmada pelos vetores `fresh` e `limit`. |

## Pseudocódigo

```text
ler até 65537 bytes; se mais de 64, falhar
para cada índice i de 0 a n − 1:
    out[n − 1 − i] = ((in[i] XOR 0x5a) + i) mod 256
escrever out[0..n−1]
```

## Como funciona

1. `readall` lê a entrada em um buffer de 65537 bytes. O byte extra
   detecta entrada acima do limite geral.
2. `main` rejeita mais de 64 bytes, o limite do enunciado.
3. O laço aplica XOR com `0x5a`, soma o índice original e grava o
   resultado na posição espelhada de um array de 64 bytes.
4. `output` escreve exatamente `n` bytes. `finish` confere `fflush` e
   `ferror`.

O assembly de `main` em `materials/mystery.asm` contém o laço diretamente:
a instrução `xor` com `0x5a`, a soma do contador e o cálculo do índice
`len - 1 - i`. Não há ofuscação.

## Reconstrução do alvo

Este `main.c` é também a fonte do alvo distribuído. O script
[rebuild-target.sh](rebuild-target.sh) compila com `-O0 -g0`, sem
símbolos locais, e confere que o ponto de entrada e os limites de `main`
coincidem com os offsets citados em `materials/traces.md`. A toolchain
registrada é GCC 16.2.1 20260810 com Binutils 2.47 em Linux x86-64. Com a
mesma toolchain, o binário é idêntico byte a byte ao distribuído.

O script só substitui o binário e a listagem depois de todas as
verificações. Uma falha de compilação ou um offset diferente preserva os
materiais anteriores. Comentários e nomes de variáveis não alteram o
código gerado; mudanças na ordem das declarações ou nas instruções alteram
os offsets e exigem atualizar os traces.

## Limites

O tempo é O(n) e a memória é fixa. O programa rejeita entradas com mais
de 64 bytes. O clube distribui apenas o alvo Linux x86-64; outros alvos
exigiriam listagens e traces próprios.
