# Manutenção das implementações

Cada implementação fica em `<id>/<linguagem>/` e possui seu próprio Makefile.
Consulte o README dessa implementação para conhecer seu compilador ou runtime.
Os testes públicos usam Python 3.10 ou superior em Unix/Linux. A verificação
de revisão usa Git.

Mantenha os checkouts `challenges` e `solutions` lado a lado. Entre no diretório
da implementação para executar:

```sh
make
make check
make clean
```

`make` compila o programa, `make check` verifica a revisão dos testes e executa
a suíte pública, e `make clean` remove o executável gerado. As implementações
em C11 e Rust geram seus executáveis no `build/` local, com avisos tratados
como erros. Rust usa `rustc` estável, sem dependências externas.

## Revisão dos testes

O arquivo [challenges.lock.json](../challenges.lock.json) registra o SHA adotado
pelo CI. Na raiz de `solutions`, execute `python3 tools/check_lock.py` para
consultá-lo. Prepare um checkout limpo de `challenges` nessa revisão.

Cada Makefile aceita outro checkout por caminho absoluto:

```sh
make check CHALLENGES_DIR=/caminho/para/challenges
```

Não descarte alterações de um checkout para satisfazer a verificação. Use outro
clone quando necessário. Para adotar testes novos, revise o commit, atualize
`revision` no manifesto e execute `make check` em cada implementação. Publique
o commit de `challenges` antes de o CI de `solutions` depender dele.

Durante alterações conjuntas, `make test` executa a suíte do checkout indicado
sem verificar o manifesto. O CI sempre usa `make check`.

Para compilar e verificar todas as referências locais contra Python, os testes
diretos de API e o runner Rust incorporado, execute na raiz de `solutions`:

```sh
python3 tools/verify_local.py
python3 tools/verify_local.py --challenge 005-base64
make -C 016-use-after-free/c sanitize
python3 ../challenges/016-use-after-free/tests/sanitizer_regression.py \
  016-use-after-free/c/implementation.c
```

O verificador compila `tools/oxide-test/Cargo.toml` no checkout vizinho de
`challenges`; o binário fica em `tools/oxide-test/target/debug/oxide-test`. Aceita
`--challenges-dir /caminho/challenges` e `--runner /caminho/oxide-test` para
usar outros caminhos; o segundo precisa corresponder aos vetores atuais.
Também confere que cópias dos headers/adaptadores fornecidos continuam iguais.
O teste de sanitizador confirma que o starter reproduz uso após liberação e que
a implementação corrigida passa nos mesmos casos, com ASan/UBSan habilitados.

Os desafios `004`–`021` têm enunciados com `draft: false`. Para atualizar seus
testes no CI, publique o commit correspondente em `challenges` e registre
esse SHA real em `challenges.lock.json`. A verificação local não
altera esse pin nem desabilita a exigência de checkout limpo de `make check`.

## Matriz de CI

O workflow executa as verificações das ferramentas e descobre todos os
Makefiles em `<id>/<linguagem>/`. Cada implementação vira um item independente
da matriz e recebe o mesmo checkout de testes públicos, na revisão do manifesto.

Para conferir a descoberta e as ferramentas, na raiz:

```sh
python3 -m pip install -r requirements-validation.txt
python3 tools/validate_community.py
python3 tools/list_solutions.py
python3 -m unittest discover -s tests -v
```

Ao adicionar uma linguagem, forneça seus próprios comandos no Makefile e
configure a instalação de dependências no workflow quando necessário. Essa
revisão fixa dos testes é independente da atualização de conteúdo no site.

## Catálogo da comunidade e website

Cada desafio possui `community.yaml`, validado pelo contrato compartilhado
[community.schema.json](../community.schema.json), pelo validador Python e pelo
website via TypeScript/Zod. Os READMEs dos desafios apenas apontam para o catálogo.
Na migração de 2026-10-20, os três READMEs não tinham contribuições cadastradas;
os três catálogos foram inicializados com `implementations: []`, sem perda de dados.

O website sincroniza a branch `main` deste repositório durante a build e associa
as entradas pelo ID do diretório, sem catálogo próprio. O snapshot
`SOLUTIONS_COMMIT` é reutilizado nas etapas do mesmo job. Nenhum link externo é
consultado pelo validador; nenhum código de participante é executado.

O workflow `deploy-website.yml` dispara o workflow existente `deploy.yml` de
`oxide-club/oxideclub.dev` somente após pushes em `main` que alterem
`*/community.yaml` ou `community.schema.json`, incluindo exclusões. PRs não
publicam o site. O destino mantém seus testes, ambiente de produção e concorrência.
A revisão fixa de `challenges.lock.json` continua independente desse snapshot.

Para ativar o disparo, configure `WEBSITE_DEPLOY_TOKEN` como segredo de Actions
em `solutions`: token de acesso pessoal fine-grained restrito a
`oxide-club/oxideclub.dev`, com **Actions: read and write**, aprovado pela
organização quando necessário. Não armazene tokens no código. O `GITHUB_TOKEN`
do repositório de origem não autoriza o dispatch entre repositórios. O workflow
não tem permissões adicionais e não faz checkout nem executa código externo.
Ausência ou falha do token faz o dispatch falhar explicitamente; acompanhe o
resultado no workflow **Deploy production** do website e repita manualmente
quando necessário.

Na primeira publicação, disponibilize o schema e os catálogos de `solutions`
antes de publicar o leitor do website. Configure o segredo antes de habilitar
o disparo. A alteração local não configura segredos nem comprova um deploy remoto;
a configuração deve ser concluída pelos mantenedores. Para prévia local,
`SOLUTIONS_SOURCE=/caminho/absoluto/solutions npm run build` no website lê este
checkout sem copiá-lo; esse override é proibido em CI e produção.
