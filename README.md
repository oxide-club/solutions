# Soluções do Oxide Club

Implementações comentadas mantidas pelo clube e um
[índice de implementações da comunidade](COMMUNITY.md).
As definições e os testes públicos ficam no repositório separado `challenges`.

> Uma possível solução comentada está disponível para estudo. Recomendamos tentar o desafio antes de consultá-la, mas você pode usá-la quando achar apropriado.

| ID estável | Referência do clube |
| --- | --- |
| `001-wc` | [Contagem de linhas, palavras e bytes em C11](001-wc/reference/README.md) |
| `002-ppm` | [Geração incremental de PPM P6 em C11](002-ppm/reference/README.md) |

Uma referência é uma abordagem possível, não a única solução correta.
Comentários e guias explicam decisões, complexidade e limites.

## Compilar e testar

Requisitos: compilador C11, Make, Python 3.10 ou superior e Git, em Unix/Linux.
Não há dependências Python externas.

Mantenha os dois checkouts lado a lado:

```text
oxide-club/
  challenges/
  solutions/
```

Na raiz de `solutions`:

```sh
make
make check
```

`make` compila as referências em `build/001-wc` e `build/002-ppm`, com avisos
habilitados e tratados como erros. `make check` confirma que `../challenges`
está na revisão fixada, sem alterações, testa o validador da revisão e executa
os mesmos testes públicos disponíveis aos participantes.

Para outro local de checkout:

```sh
make check CHALLENGES_DIR=/caminho/para/challenges
```

Cada teste público aceita o caminho do executável. As asserções não são copiadas
para este repositório. Veja os guias de cada referência para comandos individuais.
Use `make clean` para remover os dois executáveis gerados.

## Revisão dos testes e CI

[challenges.lock.json](challenges.lock.json) registra o repositório público
pretendido e o SHA completo dos testes adotados. CI valida esse arquivo,
obtém exatamente a revisão indicada e executa `make check`. Pushes e pull
requests rodam as verificações; nenhuma publicação é feita pelo workflow.

Na configuração inicial, o destino é `oxide-club/challenges`. O CI remoto
só poderá obter essa revisão depois de o repositório e o commit fixado
serem publicados. Os testes locais funcionam com os checkouts existentes.

Para conferir qual revisão preparar:

```sh
python3 tools/check_lock.py
git -C ../challenges rev-parse HEAD
```

Use um checkout limpo na revisão indicada no manifesto. Se seu checkout contém
trabalho em andamento, use outro clone e passe seu caminho em `CHALLENGES_DIR`.
Não descarte alterações para satisfazer o verificador.

Para adotar uma atualização dos testes:

1. Revise e obtenha o novo commit de `challenges` em um checkout limpo.
2. Atualize `revision` no manifesto com o SHA completo desse commit.
3. Execute `make check` e inclua a atualização do manifesto no PR.
4. Publique o commit de `challenges` antes do PR de `solutions` depender dele em CI.

Durante o desenvolvimento de mudanças conjuntas, `make test` executa os
testes do checkout indicado sem verificar o manifesto. Esse comando serve
para experimentar mudanças ainda não fixadas; o CI sempre usa `make check`.

## Participação e licença

Veja [CONTRIBUTING.md](CONTRIBUTING.md) para corrigir referências ou adicionar
seu repositório ao índice. IDs seguem `NNN-name` e correspondem ao mesmo diretório
e slug em `challenges`.

Código e documentação técnica original deste repositório usam [MIT](LICENSE).
Implementações externas listadas em `COMMUNITY.md` mantêm seus próprios autores
e licenças. As definições editoriais têm seu escopo documentado em `challenges`;
elas não recebem uma nova licença por serem referenciadas aqui.
