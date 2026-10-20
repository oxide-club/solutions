# Soluções do Oxide Club

Implementações comentadas mantidas pelo clube e um índice de soluções
publicadas pelos participantes. As definições e os testes públicos ficam em
[challenges](https://github.com/oxide-club/challenges).

## Estrutura

Os identificadores são os mesmos usados no repositório de desafios e no site.

```text
COMMUNITY.md       # Links para implementações dos participantes
NNN-name/
  linguagem/       # Implementação mantida pelo Oxide Club
    README.md      # Como compilar, testar e estudar a solução
    Makefile       # Comandos próprios desta implementação
```

Cada referência apresenta uma abordagem possível, com suas decisões e limites.
O código dos participantes permanece em seus próprios repositórios.

## Índice de desafios

| ID        | Definição                                                                                  | Referência do clube            |
| --------- | ------------------------------------------------------------------------------------------ | ------------------------------ |
| `001-wc`  | [Construa seu wc](https://github.com/oxide-club/challenges/blob/main/001-wc/index.md)      | [Estudar](001-wc/c/README.md)  |
| `002-ppm` | [Gere uma imagem PPM](https://github.com/oxide-club/challenges/blob/main/002-ppm/index.md) | [Estudar](002-ppm/c/README.md) |

## Como participar

Escolha um desafio, desenvolva sua implementação e compartilhe dúvidas e
resultados no tópico do Discord indicado na definição. Soluções parciais
são bem-vindas. Adicione seu repositório ao [COMMUNITY.md](COMMUNITY.md) por PR,
informando o desafio, sua atribuição e a linguagem utilizada.

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

Envie links de implementações para [COMMUNITY.md](COMMUNITY.md), ou proponha
correções e melhorias nas referências por pull request. Se encontrar uma
lacuna nos testes, proponha a mudança em `challenges`, onde eles são mantidos.

Veja [CONTRIBUTING.md](CONTRIBUTING.md). Código original incorporado neste
repositório usa [MIT](LICENSE); implementações externas mantêm suas próprias
licenças.
