# Soluções do Oxide Club

Implementações comentadas mantidas pelo clube e índices de soluções
publicadas pelos participantes, organizados por desafio. As definições e os testes públicos ficam em
[challenges](https://github.com/oxide-club/challenges).

## Estrutura

Os identificadores são os mesmos usados no repositório de desafios e no site.

```text
NNN-name/
  README.md        # Definição, referências do clube e orientações
  community.yaml   # Índice de implementações externas
  linguagem/       # Implementação mantida pelo Oxide Club
    README.md      # Como compilar, testar e estudar a solução
    Makefile       # Comandos próprios desta implementação
```

Cada referência apresenta uma abordagem possível, com suas decisões e limites.
O código dos participantes permanece em seus próprios repositórios.

## Índice de desafios

| ID        | Definição                                                                                  | Soluções do clube e da comunidade |
| --------- | ------------------------------------------------------------------------------------------ | ------------------------------ |
| `001-wc`  | [Construa seu wc](https://github.com/oxide-club/challenges/blob/main/001-wc/index.md)      | [Ver soluções](001-wc/README.md) |
| `002-ppm` | [Gere uma imagem PPM](https://github.com/oxide-club/challenges/blob/main/002-ppm/index.md) | [Ver soluções](002-ppm/README.md) |
| `003-rle` | [Construa um compressor RLE](https://github.com/oxide-club/challenges/blob/main/003-rle/index.md) | [Ver soluções](003-rle/README.md) |
| `004-hexdump` | [Hexdump](https://github.com/oxide-club/challenges/blob/main/004-hexdump/index.md) | [Ver soluções](004-hexdump/README.md) |
| `005-base64` | [Base64 de ida e volta](https://github.com/oxide-club/challenges/blob/main/005-base64/index.md) | [Ver soluções](005-base64/README.md) |
| `006-stream-search` | [Busca de sequência em stream](https://github.com/oxide-club/challenges/blob/main/006-stream-search/index.md) | [Ver soluções](006-stream-search/README.md) |
| `007-image-bug` | [Pixels fora do lugar](https://github.com/oxide-club/challenges/blob/main/007-image-bug/index.md) | [Ver soluções](007-image-bug/README.md) |
| `008-image-transform` | [Recorte e gire uma imagem](https://github.com/oxide-club/challenges/blob/main/008-image-transform/index.md) | [Ver soluções](008-image-transform/README.md) |
| `009-triangle` | [Rasterizador de triângulos 2D](https://github.com/oxide-club/challenges/blob/main/009-triangle/index.md) | [Ver soluções](009-triangle/README.md) |
| `010-fixed-buffer` | [Array fixo](https://github.com/oxide-club/challenges/blob/main/010-fixed-buffer/index.md) | [Ver soluções](010-fixed-buffer/README.md) |
| `011-dynamic-buffer` | [Array dinâmico](https://github.com/oxide-club/challenges/blob/main/011-dynamic-buffer/index.md) | [Ver soluções](011-dynamic-buffer/README.md) |
| `012-hash-table` | [Uma tabela hash que cresce](https://github.com/oxide-club/challenges/blob/main/012-hash-table/index.md) | [Ver soluções](012-hash-table/README.md) |
| `013-record-inspector` | [Inspetor de um formato documentado](https://github.com/oxide-club/challenges/blob/main/013-record-inspector/index.md) | [Ver soluções](013-record-inspector/README.md) |
| `014-mystery-image` | [Que arquivo é este?](https://github.com/oxide-club/challenges/blob/main/014-mystery-image/index.md) | [Ver soluções](014-mystery-image/README.md) |
| `015-mystery-program` | [O que este executável faz?](https://github.com/oxide-club/challenges/blob/main/015-mystery-program/index.md) | [Ver soluções](015-mystery-program/README.md) |
| `016-use-after-free` | [Quem liberou este buffer?](https://github.com/oxide-club/challenges/blob/main/016-use-after-free/index.md) | [Ver soluções](016-use-after-free/README.md) |
| `017-arena` | [Arena de memória alinhada](https://github.com/oxide-club/challenges/blob/main/017-arena/index.md) | [Ver soluções](017-arena/README.md) |
| `018-free-list` | [Um alocador com liberação individual](https://github.com/oxide-club/challenges/blob/main/018-free-list/index.md) | [Ver soluções](018-free-list/README.md) |
| `019-rpn` | [Calculadora RPN](https://github.com/oxide-club/challenges/blob/main/019-rpn/index.md) | [Ver soluções](019-rpn/README.md) |
| `020-expressions` | [Interpretador de expressões](https://github.com/oxide-club/challenges/blob/main/020-expressions/index.md) | [Ver soluções](020-expressions/README.md) |
| `021-bytecode` | [Uma VM de bytecode](https://github.com/oxide-club/challenges/blob/main/021-bytecode/index.md) | [Ver soluções](021-bytecode/README.md) |

Os desafios `004`–`021` cobrem novembro de 2026 a abril de 2027.
Seus enunciados estão disponíveis (`draft: false`), com três níveis por mês.
O campo `participation: flagship` identifica o principal de cada livestream;
`featured` destaca somente os exercícios trabalhados no mês atual.

## Como participar

Escolha um desafio, desenvolva sua implementação e compartilhe dúvidas e
resultados no tópico do Discord indicado na definição. Soluções parciais
são bem-vindas. Adicione sua implementação ao `community.yaml` do desafio por PR, seguindo [CONTRIBUTING.md](CONTRIBUTING.md).

Recomendamos tentar o desafio antes de consultar a referência, mas você pode
usá-la quando achar apropriado.

## Como testar um programa

Os testes públicos são os mesmos para participantes e referências do clube.
Com um checkout de `challenges`, substitua `ID_DO_DESAFIO` pelo identificador:

```sh
python3 ../challenges/ID_DO_DESAFIO/tests/test.py /caminho/para/seu-programa
```

Consulte o README de cada referência para compilação e execução. Para verificar
uma implementação mantida pelo clube, substitua também `LINGUAGEM`:

```sh
make -C ID_DO_DESAFIO/LINGUAGEM check
```

Esse comando requer um checkout de `challenges` na revisão adotada pelo CI.
O [guia de manutenção](docs/MAINTENANCE.md) explica como preparar os checkouts,
compilar as referências e atualizar essa revisão.

Para trabalhar nos dois repositórios antes de publicar uma nova revisão de
testes, execute `python3 tools/verify_local.py`. Ele compila e confere todas as
referências com os testes Python e com o runner Rust do checkout local,
preservando a proteção de revisão do CI.

## Como contribuir

Envie links de implementações para o `community.yaml` do desafio correspondente, ou proponha
correções e melhorias nas referências por pull request. Se encontrar uma
lacuna nos testes, proponha a mudança em `challenges`, onde eles são mantidos.

Veja [CONTRIBUTING.md](CONTRIBUTING.md). Código original incorporado neste
repositório usa [MIT](LICENSE); implementações externas mantêm suas próprias
licenças.

## Validar o catálogo da comunidade

O contrato está em [community.schema.json](community.schema.json). A chave
`implementations` contém referências externas; uma lista vazia é válida.
O ID vem do diretório, sem campo adicional. Consulte os campos e a ordenação
em [CONTRIBUTING.md](CONTRIBUTING.md#formato-do-catálogo).

```sh
python3 -m pip install -r requirements-validation.txt
python3 tools/validate_community.py
python3 -m unittest discover -s tests -v
```

A validação é local e não acessa os projetos vinculados. O website lê estes
catálogos durante sua build estática. O [guia de manutenção](docs/MAINTENANCE.md)
descreve a sincronização e a configuração de publicação.
