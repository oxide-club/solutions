# 007-image-bug — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição, o starter `materials/buggy.c`
e os testes públicos ficam em `007-image-bug/` no repositório separado
`challenges`.

Neste diretório (`solutions/007-image-bug/c`), com compilador C11 e o
checkout de `challenges` ao lado:

```sh
make
M=../../../challenges/007-image-bug/materials
./build/oxide-image-bug < $M/labels-3x2.ppm | cmp - $M/labels-3x2.ppm
# Sem diferença: a versão corrigida preserva a imagem retangular.
make check
```

O Makefile inclui `materials/` do desafio com `-I`, porque `ppm.h` fica
lá. Durante alterações conjuntas dos repositórios, use `make test` para
executar os testes locais sem exigir a revisão fixada no manifesto.
Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Investigação

O starter copia cada pixel de uma posição de origem para uma posição de
destino. O destino usa `y * w + x`. A origem usa `y * h + x`, com um
módulo que mantém o acesso dentro do buffer.

**Hipótese:** o cálculo da origem usa a altura como passo entre linhas.
Em uma imagem quadrada, largura e altura são iguais, e as duas fórmulas
coincidem. Em uma imagem retangular, as linhas de origem ficam
deslocadas.

**Contraexemplo:** `labels-3x2.ppm`, com largura 3 e altura 2. Previsão
para a segunda linha: o passo de origem vale 2 em vez de 3, então a linha
1 começa no pixel de índice 2, que é o azul da primeira linha. A saída
observada confirma: a segunda linha sai `azul amarelo magenta` em vez de
`amarelo magenta ciano`.

**Causa:** o passo entre linhas de um raster é a largura, não a altura.
Esse passo é chamado de stride.

**Por que o caso quadrado não revela o problema:** com `w == h`, a
expressão `y * h + x` vale o mesmo que `y * w + x` para todo pixel. O
módulo também nunca atua, porque o índice fica abaixo de `w * h`. O vetor
`image2x2` passa nas duas versões.

**Regressão:** o vetor `image3x2` compara os bytes da saída com a entrada.
Ele falha no starter e passa na correção. Os vetores `image1x4` e
`image4x1` cobrem as duas formas retangulares extremas.

## Como funciona

1. `main` rejeita argumentos e ignora `SIGPIPE`.
2. `ppm_read` valida o cabeçalho e carrega o raster da entrada.
3. O programa aloca um raster do mesmo tamanho para a saída.
4. O laço percorre as linhas e, em cada linha, as colunas. `pixel_offset`
   calcula a posição com `(y * w + x) * 3` para origem e destino.
5. `ppm_write` grava o cabeçalho e o raster. `fflush` confirma a escrita.
6. O rótulo `cleanup` libera os dois rasters em todos os caminhos.

## Decisões

A função `pixel_offset` concentra a fórmula de posição em um único lugar.
Origem e destino usam a mesma função, com a imagem correspondente. Assim,
o stride vem sempre da largura da imagem acessada.

O programa mantém a cópia pixel por pixel. Um `memcpy` do raster inteiro
seria mais rápido, mas o desafio pede para preservar o percurso por
coordenadas.

## Limites

O tempo é O(w × h). A memória é O(w × h) para os dois rasters. O formato
aceito é o subconjunto estrito de PPM P6 validado por `ppm_read`. Uma
falha pode deixar saída parcial.
