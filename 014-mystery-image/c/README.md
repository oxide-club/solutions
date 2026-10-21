# 014-mystery-image — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição, o corpus e os testes públicos
ficam em `014-mystery-image/` no repositório separado `challenges`.

ATENÇÃO: este README revela a especificação inferida. Faça a sua
investigação antes de continuar.

Neste diretório (`solutions/014-mystery-image/c`), com compilador C11 e o
checkout de `challenges` ao lado:

```sh
make
M=../../../challenges/014-mystery-image/materials
./build/oxide-mystery-image < $M/rect.px | cmp - $M/rect.ppm
# Sem diferença.
python3 $M/oracle.py 2 3 ff000000ff000000ff010203040506070809 > build/consulta.px
./build/oxide-mystery-image < build/consulta.px | tail -c 18 | od -An -tx1
# ff 00 00 00 ff 00 00 00 ff 01 02 03 04 05 06 07 08 09
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Registro da investigação

Cada linha compara pares do corpus que mudam uma propriedade por vez.

| Hipótese | Experimento | Observação | Conclusão |
|---|---|---|---|
| O cabeçalho tem assinatura fixa. | Comparar todos os `.px`. | Todos começam com `50 58 01 00`: `PX`, 1, 0. | Assinatura `PX`, versão 1, byte reservado 0. |
| As dimensões são u16 little endian. | `tall.px`, imagem 1×2. | Bytes 4 a 7: `00 01 00 02`. | Refutada. Little endian daria `01 00 02 00`. Big endian explica os dois campos. |
| Os pixels usam RGB. | `red.px`, um pixel vermelho. | Payload `00 00 ff`. | Refutada. O vermelho está no terceiro byte: a ordem é BGR. `green.px` e `blue.px` confirmam o segundo e o primeiro byte. |
| Os pixels seguem a ordem de linhas. | `rect.px`, 3×2 com pixels distintos. | Depois do pixel (0,0) vem o pixel (0,1), que no PPM fica na segunda linha. | Refutada. O arquivo avança primeiro em y: ordem de colunas. |

Validação: os vetores `fresh` da suíte usam cores e dimensões ausentes do
corpus. A conversão coincide com a especificação inferida. O comando com
`oracle.py` acima repete essa validação com uma imagem 2×3 nova.

## Especificação inferida

| Offset | Tamanho | Conteúdo |
|---|---|---|
| 0 | 2 | `PX` |
| 2 | 1 | Versão 1 |
| 3 | 1 | Reservado, zero |
| 4 | 2 | Largura, u16 big endian, de 1 a 64 |
| 6 | 2 | Altura, u16 big endian, de 1 a 64 |
| 8 | 3 × w × h | Pixels BGR em ordem de colunas |

O pixel `(x, y)` fica no offset `8 + 3 × (x × h + y)`. O arquivo termina
no último pixel.

## Como funciona

1. `main` lê toda a entrada em um buffer de 65537 bytes. O byte extra
   detecta entrada acima do limite.
2. O programa confere os quatro bytes fixos do cabeçalho.
3. `u16be` lê largura e altura. O programa rejeita zero e valores acima
   de 64.
4. O programa compara o tamanho lido com `8 + 3 × w × h`. Payload
   incompleto e bytes finais falham na mesma comparação.
5. O programa escreve o cabeçalho PPM e percorre a saída em ordem de
   linhas. Para cada pixel, calcula a origem com `x × h + y` e inverte
   B, G, R para R, G, B.
6. `fflush` confirma a escrita.

## Decisões

O cabeçalho é validado por inteiro antes de qualquer escrita. Assim, um
arquivo inválido não produz um PPM parcial.

A comparação de tamanho é uma igualdade. Ela cobre três rejeições do
enunciado de uma vez: payload incompleto, bytes finais e dimensões
incompatíveis com o conteúdo.

O laço de saída segue a ordem do PPM, não a do arquivo. Isso evita um
raster intermediário: cada pixel é lido da origem e escrito uma vez.

## Limites

O tempo é O(w × h). A memória é O(1) além do buffer fixo de entrada. O
programa não implementa o encoder nem aceita outras versões do formato.
Uma falha de escrita pode deixar saída parcial.
