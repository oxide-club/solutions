# 005-base64 — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`005-base64/` no repositório separado `challenges`.

Neste diretório (`solutions/005-base64/c`), com compilador C11:

```sh
make
printf 'foobar' | ./build/oxide-base64 encode
# Zm9vYmFy
printf 'Zm8=' | ./build/oxide-base64 decode
# fo
printf 'AB==' | ./build/oxide-base64 decode
# Diagnóstico de bits de preenchimento em stderr; status diferente de zero.
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

O alfabeto é uma string de 64 caracteres. O índice de cada símbolo é o
seu valor. A função `sextet` faz a conversão inversa com `strchr`.

Em `encode`:

1. `fread` lê um grupo de até três bytes.
2. Os bytes formam um inteiro de 24 bits. Bytes ausentes valem zero.
3. Cada fatia de seis bits escolhe um símbolo do alfabeto.
4. Com um byte, os dois últimos símbolos viram `=`. Com dois bytes, só o
   último vira `=`.
5. `fwrite` escreve os quatro símbolos.

Em `decode`:

1. `getchar` lê quatro símbolos. Um quarteto incompleto é erro.
2. Os dois primeiros símbolos nunca são padding. A função confere os dois
   no alfabeto.
3. O terceiro e o quarto símbolos definem o padding: `xx==` ou `xxx=`.
4. Os quatro valores formam 24 bits. A função calcula os três bytes.
5. Com padding, os bits descartados precisam ser zero. Caso contrário, a
   entrada não está na forma canônica e o programa rejeita.
6. A função escreve três, dois ou um byte. Depois do padding, qualquer byte
   adicional é erro.

## Decisões

O programa trabalha com um inteiro de 24 bits por grupo. Essa forma mostra
o deslocamento dos bits de modo direto, sem máscaras diferentes em cada
posição. O compilador converte as operações em instruções simples.

`sextet` rejeita o byte zero antes de chamar `strchr`. Sem essa guarda,
`strchr` encontraria o terminador da string e devolveria o índice 64.

O decodificador valida o quarteto inteiro antes de escrever. Assim, uma
entrada inválida no primeiro quarteto não produz saída. Um erro em um
quarteto posterior pode deixar saída parcial, porque o processamento é
incremental.

Quando disponível, o programa ignora `SIGPIPE`. Um pipe sem leitor produz
um erro de escrita com diagnóstico em vez de encerrar o processo pelo sinal.

## Limites

O tempo é O(n) para n bytes de entrada. A memória adicional é O(1): um
grupo de três bytes ou um quarteto de símbolos. O programa não aceita
quebras de linha, espaços ou o alfabeto URL-safe.
