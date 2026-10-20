# Contribuir com soluções

## Compartilhar uma solução

Mantenha o código em seu próprio repositório e envie um PR adicionando uma linha
à seção **Soluções da comunidade** de `<id>/README.md`, como
[001-wc/README.md](001-wc/README.md). Use o ID oficial do desafio.

Copie o modelo abaixo e substitua os dados de exemplo:

```md
| Autor | Linguagem | Solução | Estado | Observações |
| --- | --- | --- | --- | --- |
| [@usuario](https://github.com/usuario) | Rust | [Código](https://github.com/usuario/desafios/tree/main/001-wc) | Concluída | Leitura em fluxo |
```

- **Autor:** nome ou identificação pública preferida; o link para o perfil é opcional.
- **Linguagem:** linguagem utilizada na implementação.
- **Solução:** link direto para o repositório ou diretório daquela implementação.
- **Estado:** `Concluída` ou `Parcial`, declarado pelo participante. Soluções parciais são bem-vindas.
- **Observações:** descrição curta e opcional da abordagem, limitações ou instruções relevantes.

Adicione apenas a linha à tabela existente, em ordem alfabética por autor.
Se for a primeira contribuição do desafio, substitua a mensagem de que ainda
não há implementações pela tabela preenchida. Use uma linha por implementação;
para atualizar a mesma solução, edite a linha existente.

A inclusão do link não indica certificação de correção, segurança ou conclusão
pelo clube. Cada implementação externa mantém sua autoria e licença próprias;
adicionar um link não aplica MIT ao código externo.

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
a comunidade usa a tabela acima. Enquanto não houver contribuições, use
`Nenhuma implementação de participante foi indexada ainda.` nessa seção.

Cada implementação mantida pelo clube fica em `<id>/<linguagem>/`, com README e Makefile próprios.
Use nomes de linguagem em minúsculas, como `c` ou `rust`. O Makefile deve
fornecer `all`, `test`, `check` e `clean`: `test` chama o executor público do
desafio; `check` verifica primeiro a revisão de `challenges.lock.json`.
Aceite `CHALLENGES_DIR` para localizar os testes. Não copie suas asserções.

O CI descobre esses diretórios automaticamente. Configure no workflow a
instalação do compilador ou runtime quando adicionar uma linguagem que o
ambiente ainda não oferece. Execute também
`python3 -m unittest discover -s tests -v` ao alterar as ferramentas de manutenção.
