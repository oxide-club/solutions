# 003-rle — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`003-rle/` no repositório separado `challenges`.

Neste diretório (`solutions/003-rle/c`), com compilador C11:

```sh
make
printf 'AAAAABBBCC' > build/original
./build/oxide-rle encode < build/original > build/encoded
od -An -tx1 build/encoded
# 05 41 03 42 02 43
./build/oxide-rle decode < build/encoded > build/decoded
cmp build/original build/decoded
printf '\000A' | ./build/oxide-rle decode
# Diagnóstico de contagem zero em stderr; status diferente de zero.
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

`main` valida o argumento, ignora `SIGPIPE` e chama `encode` ou `decode`.
No fim, `fflush` confirma a entrega do buffer.

Em `encode`:

1. O laço lê um byte por vez com `getchar`.
2. O programa guarda o valor atual e a contagem de repetições.
3. Quando o byte muda, ou a contagem chega a 255, `pair` escreve a
   contagem e o valor, e a contagem recomeça.
4. No `EOF`, o último par pendente é escrito.

Em `decode`:

1. O laço lê a contagem. Zero é inválido.
2. O programa lê o valor. `EOF` nesse ponto é um par incompleto.
3. `putchar` repete o valor pelo número de vezes da contagem.

## Decisões

O encoder mantém um valor e uma contagem. Emite o par ao trocar de valor ou
alcançar 255 repetições e emite o último par ao chegar ao EOF. `getchar`
retorna `int`, preservando a distinção entre `0xff` e EOF. A contagem nunca
ultrapassa 255. O decoder lê pares e aceita repetições adjacentes do mesmo valor.

As operações usam os buffers de `stdio`. `ferror` distingue EOF de erro de
leitura; as escritas e o `fflush` final são verificados. Uma entrada inválida
pode deixar uma saída parcial: processamento incremental não oferece rollback.
O programa pressupõe Unix/Linux com bytes de 8 bits. Quando disponível,
`SIGPIPE` é ignorado usando `signal`, para que um pipe sem leitor produza
um erro de escrita diagnosticável em vez de encerrar o processo pelo sinal.

O encode custa O(n), o decode custa O(m + n), sendo m os bytes comprimidos
e n os bytes produzidos. Ambos usam O(1) memória adicional. RLE pode aumentar
dados sem repetições até o dobro do tamanho. Não há cabeçalho nem limite de
descompressão além das contagens de cada par.
