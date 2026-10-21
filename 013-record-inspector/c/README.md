# 013-record-inspector — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [main.c](main.c). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`013-record-inspector/` no repositório separado `challenges`.

Neste diretório (`solutions/013-record-inspector/c`), com compilador C11 e
o checkout de `challenges` ao lado:

```sh
make
./build/oxide-record-inspector < ../../../challenges/013-record-inspector/materials/example.rec
# count=2
# offset=6 type=7 length=2 data=4100
# offset=11 type=2 length=1 data=ff
printf 'REC1\002\000\007\002\000A' | ./build/oxide-record-inspector
# Diagnóstico de arquivo truncado em stderr; status diferente de zero.
make check
```

Durante alterações conjuntas dos repositórios, use `make test` para executar
os testes locais sem exigir a revisão fixada no manifesto. Consulte o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Formato REC1

| Campo | Tamanho | Conteúdo |
|---|---|---|
| Assinatura | 4 bytes | `REC1` |
| Contagem | 2 bytes | u16 little endian |
| Tipo do registro | 1 byte | u8 |
| Comprimento | 2 bytes | u16 little endian |
| Dados | Comprimento bytes | Bytes arbitrários |

Os três últimos campos se repetem uma vez por registro. Não há padding
nem bytes finais.

## Como funciona

1. `main` lê toda a entrada com `fread` em um buffer estático de
   65537 bytes. O byte extra detecta uma entrada acima do limite.
2. `validate` faz a primeira passagem. Confere a assinatura, lê a
   contagem e percorre cada registro. Em cada passo, compara o que falta
   ler com o que resta no buffer. No fim, exige `offset == length`.
3. `report` faz a segunda passagem. Escreve `count=N` e uma linha por
   registro, com offset, tipo, comprimento e os dados em hexadecimal.
4. `main` confere `ferror(stdout)` e chama `fflush`.

## Decisões

O programa usa duas passagens. A primeira só valida; a segunda só
escreve. Assim, um arquivo inconsistente não produz nenhuma linha do
relatório. Uma única passagem poderia escrever os primeiros registros e
só depois descobrir o truncamento.

Os limites usam subtração: `length - offset < 3` e
`payload > length - offset`. A soma `offset + payload` poderia transbordar
em uma implementação sem o limite de 65536 bytes. A subtração é segura
porque `offset <= length` vale em todos os pontos.

`u16le` monta o inteiro a partir dos dois bytes. O primeiro byte é o menos
significativo. O helper `read_le.h` do desafio faz o mesmo; esta
referência usa a própria função para manter o arquivo independente.

O offset impresso aponta para o byte do tipo. O primeiro registro começa
no offset 6, logo depois do cabeçalho.

## Limites

O tempo é O(n) para n bytes de entrada, com duas passagens. A memória é
O(1) além do buffer fixo de entrada. O programa rejeita entradas maiores
que 65536 bytes. Uma falha de escrita pode deixar saída parcial.
