# 024-mini-refcell — referência em Rust

O objetivo é construir interior mutability com permissões dinâmicas e
lifetimes estáticos. O nível é avançado; requer ownership, lifetimes,
`UnsafeCell`, `Deref`/`DerefMut`, `Drop` e contratos `unsafe`. O escopo é uma
célula sem sincronização entre threads, com até 255 leitores ou um escritor.

## Compilar, executar e testar

Requer Rust/Cargo estável, edição 2021, Python 3 e um checkout de `challenges`
ao lado de `solutions`. Não usa crates externas. Miri requer seu componente
instalado na toolchain selecionada.

```sh
make
./build/library/debug/examples/flow
make test
make format
make miri
make check
```

O exemplo [flow.rs](examples/flow.rs) verifica o conflito leitor/escritor,
libera o leitor e muda o valor de 7 para 9. Encerra sem saída no sucesso.
`make test` copia código e testes oficiais para `build/check`, executando
casos funcionais e clientes de compilação nesse pacote temporário.
`make miri` usa o script oficial, com detecção de vazamentos habilitada;
`MIRI_TOOLCHAIN` seleciona o canal, cujo padrão é `nightly`.
`make check` acrescenta a verificação do pin de testes; use `make test` para
trabalho conjunto. `CHALLENGES_DIR` aceita outro checkout por caminho absoluto.

## Como funciona

`UnsafeCell<T>` guarda o valor e `Cell<i16>` guarda a autorização. A célula
começa livre. A aquisição registra a autorização antes de devolver o guard;
a falha devolve apenas o erro e preserva o estado.

| Estado | Pedido de leitura | Pedido de escrita |
| --- | --- | --- |
| `0` (livre) | Muda para 1 e emite leitor | Muda para -1 e emite escritor |
| `1..254` | Incrementa e emite leitor | `ReadersActive` |
| `255` | `ReaderLimitReached` | `ReadersActive` |
| `-1` (escritor) | `WriterActive` | `WriterActive` |

O drop do leitor subtrai uma unidade; o drop do escritor retorna a zero.
Ambos só atualizam `Cell`, sem chamar código do usuário nem panicar.

## Decisões e auditoria de segurança

Nenhuma referência ao valor é criada durante a aquisição. `Deref` do leitor
cria `&T` somente depois de existir um leitor contado, o que impede qualquer
escritor. No escritor, `Deref` e `DerefMut` usam a permissão exclusiva; o borrow
do próprio guard impede acessos incompatíveis pelo mesmo guard. O lifetime
das referências retornadas é o do borrow do guard, sem extração pública de
referências que sobrevivam a ele. Os campos privados e a ausência de `Clone`
impedem fabricar ou duplicar permissões. Os comentários `SAFETY` relacionam
cada conversão de raw pointer ao invariante correspondente.

O limite de 255 é verificado antes da soma; a recusa não altera o contador.
Um leitor vivo sempre corresponde a uma unidade, portanto sua subtração não
faz underflow. RAII libera permissões em retorno normal, retorno antecipado
e unwind; não há recuperação lógica automática de alterações feitas antes de
um panic.

Se `mem::forget` impedir o drop do guard, sua autorização permanece ocupada.
Um leitor esquecido bloqueia escritores; um escritor esquecido bloqueia ambos.
Isso pode impedir progresso, mas nunca concede uma permissão conflitante.
O conteúdo continua sendo destruído uma vez com a célula quando já não há
referências utilizáveis. A segurança não depende de todos os guards rodarem
seus destrutores.

`Cell` e `UnsafeCell` tornam a célula `!Sync`. Pelos auto traits, transferir
ownership é permitido quando `T: Send`, sem `unsafe impl Send`. Os guards
contêm `&MiniRefCell<T>` e são `!Send`/`!Sync`, pois a célula é `!Sync`.
Não há sincronização que permitiria compartilhar essa representação entre
threads. Os testes de compilação também usam conteúdo `Sync` e `!Send` para
recusar uma condição incorreta baseada apenas em `T: Sync`.

## Limites

Todas as operações de controle custam O(1); a célula usa o espaço de `T` e
um contador, além de possível padding. Cada guard guarda uma referência.
Não há clonagem, projeção ou recuperação de guards esquecidos. O mecanismo
controla borrows em uma thread e não substitui um mutex. Miri cobre os casos
executados e complementa a análise da interface segura.

## Verificação registrada

Em 25/10/2026, Linux x86-64, Rust/Cargo 1.98.1 e Python 3.14.8, `make test`
passou cinco casos de contrato e os pares de compilação. O starter falhou
nos cinco casos funcionais antes da implementação.

`make miri` passou os cinco casos, incluindo unwind, contador limite e guards
esquecidos, com Miri 0.1.0 (`12433c307b`, 24/10/2026), rustc
1.101.0-nightly. A detecção de vazamentos permaneceu habilitada.
