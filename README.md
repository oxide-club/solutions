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
