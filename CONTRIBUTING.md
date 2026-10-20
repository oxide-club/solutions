# Contribuir com soluções

Para compartilhar sua implementação, envie um PR para
[COMMUNITY.md](COMMUNITY.md), mantendo o código em seu próprio repositório.
Informe o ID do desafio, sua atribuição, linguagem e link.

Para corrigir uma referência mantida pelo clube:

1. Leia a definição e os testes no repositório `challenges`.
2. Explique o problema e forneça uma entrada ou cenário que o reproduza.
3. Preserve comentários que expliquem decisões e atualize-os com o código.
4. Execute `make -C <id>/<linguagem> check` usando a revisão de `challenges.lock.json`.

Se o erro revelar uma lacuna nos testes públicos, proponha a correção no
repositório `challenges`. Não crie uma cópia privada dos testes aqui.
Contribuições de código destinadas à incorporação nas referências usam
[MIT](LICENSE). O índice não modifica a licença dos projetos externos.

IDs como `001-wc` são permanentes e correspondem ao mesmo diretório e slug
em `challenges`. Não renumere os desafios nem crie referências para definições
que ainda não existem.

Cada implementação fica em `<id>/<linguagem>/`, com README e Makefile próprios.
Use nomes de linguagem em minúsculas, como `c` ou `rust`. O Makefile deve
fornecer `all`, `test`, `check` e `clean`: `test` chama o executor público do
desafio; `check` verifica primeiro a revisão de `challenges.lock.json`.
Aceite `CHALLENGES_DIR` para localizar os testes. Não copie suas asserções.

O CI descobre esses diretórios automaticamente. Configure no workflow a
instalação do compilador ou runtime quando adicionar uma linguagem que o
ambiente ainda não oferece. Execute também
`python3 -m unittest discover -s tests -v` ao alterar as ferramentas de manutenção.
