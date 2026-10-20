# Contribuir com soluções

## Compartilhar uma implementação

1. Escolha um desafio publicado pelo Oxide Club.
2. Desenvolva sua implementação, completa ou parcial.
3. Publique o código em um repositório público próprio, em qualquer serviço.
4. Faça um fork de `oxide-club/solutions`.
5. Localize `{challenge_id}/community.yaml`, usando o ID canônico, como `001-wc`.
6. Adicione uma entrada com seus dados à lista `implementations`.
7. Abra um pull request com a alteração.
8. Aguarde a revisão dos mantenedores.

Não é necessário reproduzir a estrutura de diretórios do clube em seu repositório.
O código permanece hospedado por você; o catálogo armazena apenas referências.

### Formato do catálogo

Exemplo completo com **dados inteiramente fictícios** (não copie como contribuição real):

```yaml
implementations:
  - author: "Pessoa Exemplo"
    authorUrl: "https://example.org/pessoa-ficticia"
    language: "Rust"
    url: "https://example.org/repositorio-ficticio/wc/src/main.rs"
    status: "partial"
    notes: "Exemplo fictício: leitura em fluxo; contagem de palavras em desenvolvimento."
```

Campos obrigatórios:

- `author`: string com o nome ou identificador público escolhido; não precisa ser username do GitHub.
- `language`: string com a linguagem utilizada, inclusive linguagens sem referência oficial.
- `url`: URL HTTPS pública apontando diretamente para a implementação, preferencialmente seu diretório ou arquivo de código.
- `status`: `completed` (declarada concluída) ou `partial` (parcial ou em desenvolvimento).

Campos opcionais:

- `authorUrl`: URL HTTPS do perfil ou página pessoal; sem ela, o autor aparece sem link.
- `notes`: texto breve sobre abordagem, limitações ou decisões técnicas; não use HTML.

Os catálogos usam YAML 1.2. Nomes e observações aceitam Unicode. Use apenas os campos documentados e URLs
HTTPS válidas, sem credenciais. Não inclua as referências oficiais neste arquivo.
O contrato de tipos está em [community.schema.json](community.schema.json).

Mantenha a lista em ordem crescente por `author`, usando `language` como segundo
critério: comparação exata dos pontos de código Unicode, sensível a maiúsculas e
acentos, sem depender da configuração regional. Use a mesma grafia para seu nome.
Múltiplas implementações do mesmo autor são aceitas; cada URL deve ser única
dentro do desafio. Para atualizar uma contribuição, edite sua entrada existente
em vez de criar uma duplicata. Sem contribuições, use `implementations: []`.

Execute os comandos de validação do [README](README.md#validar-o-catálogo-da-comunidade)
antes de enviar o PR. Não mantenha uma segunda lista no README do desafio.

### Revisão e licenciamento

O clube poderá verificar a pertinência, a existência de código correspondente
e a conformidade com o formato. O estado é declarado pelo participante.
A inclusão não garante correção, segurança, desempenho ou conclusão.

Implementações externas pertencem aos respectivos autores e mantêm suas próprias
licenças. A inclusão de um link não altera o licenciamento desse código.
Código incorporado às implementações oficiais permanece sujeito à licença
[MIT](LICENSE) aplicável a este repositório.

## Corrigir uma referência do clube

Para corrigir uma referência mantida pelo clube:

1. Leia a definição e os testes no repositório `challenges`.
2. Explique o problema e forneça uma entrada ou cenário que o reproduza.
3. Preserve comentários que expliquem decisões e atualize-os com o código.
4. Execute `make -C <id>/<linguagem> check` usando a revisão de `challenges.lock.json`.

Se o erro revelar uma lacuna nos testes públicos, proponha a correção no
repositório `challenges`. Não crie uma cópia privada dos testes aqui.
Contribuições de código destinadas à incorporação nas referências usam
[MIT](LICENSE). O índice não modifica a licença dos projetos externos.

## Estrutura por desafio

IDs como `001-wc` são permanentes e correspondem ao mesmo diretório e slug
em `challenges`. Não renumere os desafios nem crie referências para definições
que ainda não existem.

Cada desafio tem um `<id>/README.md` com link para sua definição oficial,
uma seção **Referências do clube** e uma seção **Soluções da comunidade**.
As referências do clube apontam para os READMEs das linguagens disponíveis;
a comunidade é indexada exclusivamente em `<id>/community.yaml`. O README
aponta para esse catálogo e para este guia, sem duplicar suas entradas.

Cada implementação mantida pelo clube fica em `<id>/<linguagem>/`, com README e Makefile próprios.
Use nomes de linguagem em minúsculas, como `c` ou `rust`. O Makefile deve
fornecer `all`, `test`, `check` e `clean`: `test` chama o executor público do
desafio; `check` verifica primeiro a revisão de `challenges.lock.json`.
Aceite `CHALLENGES_DIR` para localizar os testes. Não copie suas asserções.

O CI descobre esses diretórios automaticamente. Configure no workflow a
instalação do compilador ou runtime quando adicionar uma linguagem que o
ambiente ainda não oferece. Execute também
`python3 -m unittest discover -s tests -v` ao alterar as ferramentas de manutenção.
