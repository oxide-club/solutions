# 027-pe-inspector — referência em C

Esta referência implementa o subconjunto PE32+ AMD64 do
[contrato](https://github.com/oxide-club/challenges/blob/main/027-pe-inspector/index.md).
O objetivo é estudar limites de parsing e a diferença entre RVA e offset no
arquivo. O público deve conhecer C, inteiros sem sinal, little-endian e
intervalos semiabertos. O programa inspeciona bytes; não executa o PE.

## Compilar, usar e verificar

Requer compilador C11, Make e Python 3.10 ou superior, com o checkout
`challenges` ao lado de `solutions`. A execução foi verificada em Linux x86-64.

```sh
make
python3 ../../../challenges/027-pe-inspector/materials/generate.py build/fixtures
./build/oxide-pe-inspector build/fixtures/base.bin --rva 0x1010 --size 1
make test
make check
```

A consulta produz `kind="section_raw"`, `section_index=0` e
`file_offset="0x210"`. `make test` usa os testes locais; `make check` também
exige a revisão de testes e o checkout limpo definidos pelo
[guia de manutenção](../../docs/MAINTENANCE.md).
O gerador e os resultados independentes são do clube, CC0 1.0, mantidos em
`challenges/027-pe-inspector/materials/` e `tests/cases.json`; não são cópias
obtidas de executáveis de terceiros.

## Como funciona

1. `main` valida a consulta, lê até 64 MiB mais um byte e distingue falha de
   I/O de tamanho excessivo. O byte extra detecta arquivos acima do limite.
2. `parse` valida DOS, PE/COFF, optional header e tabela de seções, nessa
   ordem. Só depois decodifica campos dentro de cada intervalo validado.
3. Cada seção conserva os oito bytes do nome e os campos virtuais e raw.
   A validação rejeita invasão dos cabeçalhos, truncamento e sobreposição.
4. `print_query` encontra a região do início e compara o fim da consulta
   com o fim dessa região. Uma lacuna inicial é `unmapped`, mesmo quando
   uma parte posterior da consulta alcança uma seção.
5. O inventário só é escrito após validar todo o arquivo. Cada diagnóstico
   é um JSON em stderr com código, campo e offset. Os status são 0 para
   consulta classificada, 2 para uso, 3 para rejeição e 4 para I/O.

## Layout e evidências do fixture base

Este recorte anota os bytes produzidos pelo gerador oficial; os intervalos
foram derivados dos campos, sem carregar a imagem:

| Offset | Bytes | Significado |
| --- | --- | --- |
| `0x00` | `4d 5a` | DOS, assinatura `MZ`. |
| `0x3c` | `80 00 00 00` | Offset PE `0x80`. |
| `0x80` | `50 45 00 00 64 86 01 00` | PE, AMD64, uma seção. |
| `0x94` | `70 00` | Optional header de 112 bytes. |
| `0xd4` | `00 02 00 00` | Headers terminam em `0x200`. |
| `0x108` | `2e 74 65 78 74 00 00 00` | Nome `.text` em oito bytes. |
| `0x110` | `00 03 00 00 00 10 00 00` | VirtualSize `0x300`, RVA `0x1000`. |
| `0x118` | `00 02 00 00 00 02 00 00` | RawSize `0x200`, RawPointer `0x200`. |

Para conferir os bytes: `od -Ax -tx1 -N 304 build/fixtures/base.bin`.
A tabela termina em `0x130`; headers ocupam `[0,0x200)` e raw ocupa
`[0x200,0x400)`. A seção consulta `[0x1000,0x1300)`, com prefixo raw
`[0x1000,0x1200)` e zero-fill `[0x1200,0x1300)`.
Logo `0x11ff` com tamanho 1 é raw; com tamanho 2 cruza região;
`0x1200` é zero-fill e `0x1300` é unmapped. Essas são previsões do contrato;
`make test` compara os resultados com os valores independentes dos 76 casos.

## Decisões

`contains` usa `length <= size - offset` depois de conferir `offset <= size`.
Assim, não soma um offset não confiável antes da validação. Cada leitura
little-endian monta bytes; não converte o arquivo em structs C alinhadas.

Offsets de entrada têm no máximo 32 bits, o tamanho do optional header tem
16 bits e a contagem validada não passa de 96. As somas posteriores usam
`uint64_t`, onde seus máximos cabem. O limite exclusivo virtual pode ser
`2^32`. Um offset só chega à leitura após caber no buffer de até 64 MiB;
a conversão para `size_t` então preserva o valor no ambiente declarado.

A comparação de cada seção com as anteriores custa O(s²), para s <= 96,
e mantém os índices originais, inclusive quando a tabela não está ordenada.
Intervalos que apenas se tocam não se sobrepõem. Raw além de VirtualSize
não ganha RVAs; zero-fill não ganha offsets no arquivo. Diretórios são
campos opacos, sem tentar interpretá-los como RVAs.

## Limites

Memória O(n + s), com uma reserva de 64 MiB mais um byte para o arquivo;
leitura O(n), validação O(s²) e consulta O(s). Não valida imports,
alinhamentos, checksum ou aceitação pelo loader. O JSON segue exatamente
o esquema do desafio, incluindo nomes binários em hexadecimal; não há
serialização de texto arbitrário da entrada. Falha de escrita pode deixar
stdout parcial. Não foi verificado runtime Windows nem comparação com
um leitor de produção nesta referência.
