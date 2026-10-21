# 002-ppm — referência mantida pelo Oxide Club

Uma possível solução comentada está disponível para estudo. Recomendamos
tentar o desafio antes de consultá-la, mas você pode usá-la quando achar apropriado.

O código está em [main.c](main.c). A definição e os testes públicos ficam
no diretório `002-ppm/` do repositório separado `challenges`.

Neste diretório (`solutions/002-ppm/c`):

```sh
make
./build/oxide-ppm > build/imagem.ppm
wc -c < build/imagem.ppm
# 196623
make check
```

## Como funciona

1. `main` escreve o cabeçalho ASCII `P6\n256 256\n255\n` com `fputs`.
2. O laço externo percorre as linhas (`y`). O interno percorre as colunas
   (`x`).
3. Para cada pixel, o programa monta três bytes: vermelho igual a `x`,
   verde igual a `y` e azul igual a zero.
4. `fwrite` grava os três bytes binários. Cada escrita é verificada.
5. No fim, `fflush` confirma a entrega do buffer.

## Decisões

O cabeçalho é texto ASCII. Os canais que vêm depois são bytes binários RGB;
usar `printf` com números decimais para os pixels produziria outro formato.
O laço externo percorre as linhas, e o interno percorre as colunas.

A implementação mantém somente três bytes de pixel por vez. Para uma imagem
de largura W e altura H, a abordagem exige O(W × H) operações e O(1) espaço
adicional; neste desafio, ambas as dimensões são fixas em 256.

Cada escrita é verificada, assim como o `fflush` final, pois uma falha pode
aparecer apenas ao descarregar o buffer. O programa pressupõe Unix/Linux e
bytes de 8 bits e não implementa dimensões configuráveis ou PPM P3.
