# 009-triangle — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`009-triangle/` no repositório separado `challenges`.

Neste diretório (`solutions/009-triangle/c`), com compilador C11 e o
checkout de `challenges` ao lado:

```sh
make
M=../../../challenges/009-triangle/materials
./build/oxide-triangle < $M/triangle.txt | cmp - $M/triangle.ppm
# Sem diferença.
./build/oxide-triangle < $M/triangle.txt | tail -c 27 | od -An -tx1
# aa 2b 2b 55 80 2b 00 00 00 55 2b 80 00 00 00 ...
make check
```

O Makefile inclui `materials/` do desafio com `-I`, porque `ppm.h` fica
lá. Durante alterações conjuntas dos repositórios, use `make test` para
executar os testes locais sem exigir a revisão fixada no manifesto.
Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

1. `read_integer` lê um token: sinal `-` opcional e dígitos, separado por
   whitespace. `read_bounded` confere o intervalo de cada campo.
2. `main` lê `W H N` e aloca o canvas com `calloc`. O raster começa preto.
3. Para cada triângulo, `read_triangle` lê os três vértices com posição e
   cor.
4. `main` calcula a área dobrada com `edge`. Área zero: o triângulo é
   ignorado. Área negativa: `main` troca os vértices b e c, com as cores,
   e inverte o sinal.
5. `rasterize` percorre todos os pixels do canvas. Para cada centro de
   pixel, calcula as três funções de aresta e aplica a regra de empate.
6. Nos pixels cobertos, `rasterize` interpola cada canal com os pesos
   baricêntricos e arredonda.
7. Depois do último triângulo, `main` exige que só reste whitespace.
8. `ppm_write` grava a imagem. `fflush` confirma a escrita.

## Aritmética inteira

O enunciado amostra o pixel `(x, y)` no centro `(x + 0.5, y + 0.5)`. Para
evitar ponto flutuante, o programa dobra todas as coordenadas: o centro
vira `(2x + 1, 2y + 1)` e cada vértice vira `(2vx, 2vy)`. A função `edge`
devolve `2·E`. A área também sai dobrada. Os pesos são razões
`E_i / A = (2·E_i) / (2·A)`, então o fator comum desaparece.

Os produtos usam `int64_t`. Com coordenadas até ±1024 e canvas até 256,
cada produto fica abaixo de 10⁷. A soma ponderada das cores multiplica
esse valor por 255 e por três termos. O total fica abaixo de 10¹⁰, que
não cabe em 32 bits, mas cabe com folga em 64 bits. O vetor
`coordinate-limits-overflow` detecta produtos de 32 bits.

O arredondamento `floor(valor + 0.5)` vira a divisão inteira
`(2·soma + área) / (2·área)`. Todos os termos são não negativos dentro de
um pixel coberto, então a divisão inteira é o piso. O exemplo
`triangle1` produz o pixel `(170, 43, 43)` com essa fórmula.

## Regra de empate das bordas

Quando o centro de um pixel está exatamente sobre uma aresta, `E = 0`.
Duas faces adjacentes percorrem essa aresta em sentidos opostos: uma vai
de `u` para `v`, a outra de `v` para `u`. A regra do enunciado aceita o
ponto apenas quando `vy < uy`, ou `vy = uy` e `vx > ux`. Só um dos dois
sentidos satisfaz essa condição. Por isso, cada pixel sobre a borda
compartilhada pertence a exatamente uma face. Os vetores `triangle5` e
`triangle6` verificam esse comportamento com as duas ordens de desenho.

## Decisões

`rasterize` examina o canvas inteiro para cada triângulo. Essa escolha
mantém o código curto. Uma iteração restrita ao retângulo delimitador do
triângulo é a extensão sugerida pelo enunciado e reduz o custo para
triângulos pequenos.

A normalização da orientação troca os vértices inteiros, com as cores.
Assim, cada cor continua associada ao seu vértice depois da troca.

A validação do fim da entrada vem depois do último triângulo e antes da
escrita. Tokens extras produzem erro sem saída.

## Limites

O tempo é O(N × W × H). A memória é O(W × H) para o canvas. O programa
não implementa clipping explícito: pixels fora do canvas simplesmente não
são percorridos. Uma falha pode deixar saída parcial.
