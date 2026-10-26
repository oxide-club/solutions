# 023-mini-vec — referência em Rust

O objetivo desta biblioteca é separar armazenamento alocado, posições
inicializadas e ownership de valores. O nível é avançado; requer generics,
`Drop`, raw pointers, alinhamento, `Layout` e leitura de contratos `unsafe`.
O escopo é um vetor com inserção, remoção final e consulta imutável; ZSTs
(tipos de tamanho zero) são recusados.

## Compilar, executar e testar

Requer Rust/Cargo estável, edição 2021, Python 3 e `challenges` ao lado de
`solutions`. Não usa crates externas. Miri requer uma toolchain com o componente
instalado, conforme o guia do desafio.

```sh
make
./build/library/debug/examples/flow
make test
make format
make miri
make check
```

O exemplo [flow.rs](examples/flow.rs) insere 10, 20 e 30, confere comprimento
3/capacidade 4 e remove 30. Encerra sem saída quando as asserções passam.
`make test` prepara `build/check` com a implementação e os testes oficiais e
executa casos nativos e pares de compilação. `make miri` usa o script oficial
nesse pacote temporário; a detecção de vazamentos permanece habilitada.
`MIRI_TOOLCHAIN` seleciona o canal, cujo padrão é `nightly`.
`make check` confere também o pin dos testes; para desenvolvimento conjunto,
use `make test`. `CHALLENGES_DIR` aceita outro checkout por caminho absoluto.

## Como funciona

1. `new` recusa ZST antes de criar o adaptador, que aloca seu estado com `Rc`.
   Um vetor vazio guarda um ponteiro alinhado de `NonNull::dangling()`.
2. Se não houver espaço, `grow` calcula capacidade e layout, solicita outro
   buffer e só então transfere os valores e libera o armazenamento antigo.
3. `push` escreve no primeiro slot livre e incrementa `len` depois da escrita.
4. `pop` reduz `len` antes de ler o último valor, transferindo seu ownership.
5. `get` verifica o índice e retorna uma referência vinculada a `&self`.
6. `Drop` instala um guard para liberar o buffer e destrói valores do último
   ao primeiro. O guard continua ativo se o destrutor de um valor panicar.

## Decisões e auditoria de segurança

| Invariante | Como é estabelecido e preservado |
| --- | --- |
| `0 <= len <= cap` | Inserção cresce antes de escrever; remoção verifica vazio |
| Valores vivos somente em `0..len` | Escrita publica `len`; remoção retira `len` antes de transferir |
| `cap == 0` não possui buffer | Nenhum acesso ou liberação usa o ponteiro sentinela |
| `cap > 0` possui alocação alinhada | O allocator recebe `Layout::array::<T>(cap)` validado |
| Uma responsabilidade por valor | Crescimento move bytes sem clonar nem destruir a origem |
| Referências válidas até seu último uso | `get` vincula a referência a `&self`; mutações exigem `&mut self` |

`layout_for` rejeita overflow e layouts acima do limite de `Layout` antes do
allocator. A capacidade vai de zero a um e depois dobra com `checked_mul`.
Falha de cálculo ou recusa de alocação devolve o item recebido intacto; nenhum
campo nem elemento anterior muda. O adaptador `support.rs` é preservado.

Na realocação, origem e destino são alocações distintas. A cópia não
sobreposta transfere os `len` valores; o buffer antigo é liberado sem executar
seus destrutores. Não há ponto de panic de código do usuário entre a cópia e
a atualização dos campos. O layout antigo é reconstruído da capacidade
validada. Cada bloco `unsafe` documenta a origem de sua permissão.

No drop, o contador diminui antes de chamar o destrutor de cada valor. Se ele
panicar, não será chamado outra vez; os destrutores restantes podem ser
omitidos. `BufferRelease` libera o armazenamento bruto uma vez no unwind.
Seu destrutor só chama o adaptador com o ponteiro vivo e layout original,
sem chamar código de `T`. O adaptador fornecido conta operações e não executa
callbacks do usuário. A política admite vazamento dos recursos externos dos
valores cujos destrutores não rodaram, conforme o contrato.

`PhantomData<T>` expressa ownership dos valores, embora o armazenamento seja
acessado por raw pointer. O `Rc` interno do adaptador restringe o uso desta
referência a uma thread; não acrescentamos implementações manuais de auto traits.

## Limites

Consulta e remoção custam O(1). Uma inserção com crescimento custa O(n);
inserções sucessivas têm custo amortizado O(1), desconsiderando o custo de
mover/destruir `T`. O buffer usa O(cap) espaço e o crescimento mantém os dois
buffers temporariamente. Não há redução de capacidade, indexação mutável,
iteração, tratamento de ZST nem recuperação de OOM que aborte o processo.
Miri examina execuções concretas; não prova a segurança geral da abstração.

## Verificação registrada

Em 25/10/2026, Linux x86-64, Rust/Cargo 1.98.1 e Python 3.14.8, `make test`
passou oito casos de contrato, um de recusa de ZST sem alocação, um do
adaptador e os pares de compilação. Antes da implementação, os oito casos de
contrato falharam por `todo!`.

`make miri` passou os oito casos de contrato e o caso de ZST, com Miri
0.1.0 (`12433c307b`, 24/10/2026), nightly instalado, rustc 1.101.0-nightly.
O recorte inclui alinhamento, ownership, layout, recusas e unwind em cada
posição da bomba de drop. Não houve desativação da detecção de vazamentos.
