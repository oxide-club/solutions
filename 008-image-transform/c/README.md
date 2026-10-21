# 008-image-transform — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`008-image-transform/` no repositório separado `challenges`.

Neste diretório (`solutions/008-image-transform/c`), com compilador C11 e
o checkout de `challenges` ao lado:

```sh
make
M=../../../challenges/008-image-transform/materials
./build/oxide-image-transform rotate < $M/labels-3x2.ppm | head -c 11
# P6
# 2 3
# 255
./build/oxide-image-transform crop 1 0 2 2 < $M/labels-3x2.ppm | tail -c 12 | od -An -tx1
# 00 ff 00 00 00 ff ff 00 ff 00 ff ff
make check
```

O Makefile inclui `materials/` do desafio com `-I`, porque `ppm.h` fica
lá. Durante alterações conjuntas dos repositórios, use `make test` para
executar os testes locais sem exigir a revisão fixada no manifesto.
Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

1. `main` identifica a operação pelos argumentos. `rotate` não recebe
   números. `crop` recebe X, Y, W e H.
2. `parse_number` converte cada número. Aceita só dígitos e valores até
   256. W e H precisam ser positivos.
3. `ppm_read` carrega a imagem de entrada.
4. Para `crop`, o programa confere que o retângulo cabe na imagem. Para
   `rotate`, a saída recebe largura `h` e altura `w`.
5. O programa aloca o raster da saída e chama `crop` ou `rotate`.
6. `ppm_write` grava o resultado. `fflush` confirma a escrita. O rótulo
   `cleanup` libera os dois rasters.

## Mapeamento de coordenadas

As duas operações usam mapeamento inverso. O laço percorre os pixels da
**saída** e busca, para cada um, o pixel de origem. Cada pixel de destino
é escrito uma única vez, e nenhum fica sem valor.

| Operação | Saída `(x, y)` recebe a entrada | Dimensões da saída |
|---|---|---|
| `crop X Y W H` | `(X + x, Y + y)` | W × H |
| `rotate` | `(y, h − 1 − x)` | h × w |

Na rotação horária, a última linha da entrada (`y = h − 1`) vira a
primeira coluna da saída (`x = 0`). A primeira coluna da entrada vira a
primeira linha da saída. Por isso, largura e altura trocam de lugar.

## Decisões

A validação dos argumentos vem antes da leitura da imagem. Um argumento
inválido produz o diagnóstico sem consumir `stdin`.

Os limites do recorte usam subtrações: `width > in.w − x0` em vez de
`x0 + width > in.w`. A forma com soma poderia transbordar se os valores
não fossem limitados a 256. A forma com subtração é segura para qualquer
`size_t`, depois da verificação `x0 <= in.w`.

`crop` e `rotate` são funções separadas. Cada uma expressa sua fórmula em
uma linha. A função `pixel_offset` concentra o cálculo de posição e usa a
largura da imagem acessada como stride.

## Limites

O tempo é proporcional ao número de pixels da saída. A memória é
O(w × h) para os dois rasters. O programa não combina as operações, não
interpola e não aceita outras rotações. Uma falha pode deixar saída
parcial.
