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

## Matriz de CI

O workflow executa as verificações das ferramentas e descobre todos os
Makefiles em `<id>/<linguagem>/`. Cada implementação vira um item independente
da matriz e recebe o mesmo checkout de testes públicos, na revisão do manifesto.

Para conferir a descoberta e as ferramentas, na raiz:

```sh
python3 tools/list_solutions.py
python3 -m unittest discover -s tests -v
```

Ao adicionar uma linguagem, forneça seus próprios comandos no Makefile e
configure a instalação de dependências no workflow quando necessário. Essa
revisão fixa dos testes é independente da atualização de conteúdo no site.
