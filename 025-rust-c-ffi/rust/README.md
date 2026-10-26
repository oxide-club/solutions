# 025-rust-c-ffi — referência em Rust e C

O objetivo é estudar ABI, ownership e erros em uma integração C → Rust → C.
O nível é avançado; requer ponteiros e buffers em C, ownership e `unsafe` em
Rust, compilação separada e linking. O escopo é uma transformação didática:
`rotate_left(src[i] ^ key, 1)`. Não é criptografia.

## Compilar, executar e testar

Requer Linux x86-64/glibc, Rust estável com edição 2021, compilador C11,
`ar`, `nm`, `readelf`, Python 3 e runtime LeakSanitizer do compilador C.
O checkout `challenges` deve ficar ao lado de `solutions`.

```sh
make
make test
./build/check/client
./build/check/helper
./build/check/layout
nm --defined-only build/check/client
readelf -hW build/check/client
make format
make check
```

`make` gera bibliotecas Rust estática/rlib de distribuição, sem gancho de teste.
`make test` prepara o layout de fontes esperado pelo harness em `build/source`
e usa `tests/check.py --source ...` oficial. O harness compila os clientes C e
Rust, usa `panic=unwind`, verifica layout e símbolos e executa LeakSanitizer.
`make check` exige também o pin de revisão; use `make test` em desenvolvimento
conjunto. `CHALLENGES_DIR` aceita outro checkout por caminho absoluto.

O cliente oficial contém o exemplo com chave zero: `00 01 80 ff` produz
`00 02 01 ff`. Ele também verifica sentinelas, erros e o caminho C → Rust → C.
O [cabeçalho](oxide.h) documenta todas as pré-condições de memória e status.

## Como funciona

1. `oc_create` verifica o slot de saída, deixa-o nulo e captura unwind durante
   a criação de `Box<OcContext>`. Publica o handle somente no sucesso.
2. `oc_transform` rejeita nulidade incompatível com comprimento e limites
   acima de `isize::MAX`, sem acessar os buffers.
3. Consulta poison e depois capacidade. Capacidade insuficiente não escreve
   e informa o tamanho necessário.
4. Dentro de `catch_unwind`, passa o checkpoint e chama o helper C para cada
   byte XOR com a chave. Somente a parte correspondente à entrada é escrita.
5. Panic capturado envenena o contexto e retorna `InternalPanic`; novas
   chamadas válidas retornam `Poisoned`. `oc_destroy` recebe ownership de volta
   e libera a alocação Rust, inclusive de contextos envenenados.

## Decisões e auditoria da ABI

| Recurso | Ownership e validade |
| --- | --- |
| Slot `out` | Pertence ao cliente; slot vivo, alinhado e gravável durante criação |
| Handle `ctx` | Rust aloca; cliente guarda; `oc_destroy` libera uma vez |
| `src` | Cliente mantém bytes vivos, legíveis e inicializados até o retorno |
| `dst` | Cliente mantém região viva e gravável até o retorno |
| `OcResult` | Valor com ABI C; nenhum ponteiro para estado interno |

A região do contexto e os buffers não vazios não se sobrepõem. As chamadas
sobre um contexto são serializadas; nenhuma referência é retida depois do
retorno. Pré-condições de extensão dos buffers só se aplicam aos escalares
aceitos. Nulidade e limites escalares precedem poison; isso permite rejeitar
comprimentos impossíveis sem formar slices nem acessar buffers. `ctx` não
nulo continua exigindo handle vivo, mesmo com escalares rejeitados.

| Status | `written` | `required` | Destino |
| --- | --- | --- | --- |
| `Ok` | Comprimento da entrada | Comprimento da entrada | Prefixo transformado |
| `InvalidArgument` | 0 | 0 | Intacto |
| `BufferTooSmall` | 0 | Comprimento da entrada | Intacto |
| `InternalPanic` | 0 | 0 | Pode estar parcial; ignore os bytes |
| `Poisoned` | 0 | 0 | Intacto |

`OcResult` usa `repr(C)` e campos `i32/usize/usize`. No ambiente LP64 adotado,
o tamanho é 24, alinhamento 8 e offsets 0/8/16. Não se exportam referências,
`Vec`, `String` nem a representação do contexto. A rotação promove o byte para
`unsigned int`, combina os operadores bitwise e converte o resultado a `u8`.

Cada acesso `unsafe` declara sua pré-condição: slot gravável na criação,
handle vivo/exclusivo na transformação e origem única da alocação na
liberação. O laço só acessa índices válidos nas duas regiões e não cria
referências para buffers nulos vazios. O helper C aceita qualquer byte e não
faz unwind nem `longjmp`.

O payload de um panic também pode ter um destrutor que panica. A referência
captura esse segundo unwind ao descartar o payload original e, nesse caso
excepcional, retém o novo payload para que nenhum unwind atravesse a ABI.
O payload estático do gancho oficial é liberado normalmente. O contexto tem
somente `u8` e `bool`, portanto sua destruição não chama código arbitrário.
O gancho de panic permanece exatamente como o suporte oficial e só existe
no build com `test_hooks`.

## Limites

A transformação custa O(n) e não aloca um buffer auxiliar. O contexto ocupa
espaço constante. Não se detectam handles falsos/expirados, sobreposição ou
regiões inválidas; violam pré-condições e podem causar UB. `catch_unwind` não
captura abort, sinais, OOM abortante ou toda exceção estrangeira. Um segundo
panic no destrutor do payload pode reter seus recursos. Não há alegação de
Miri para esta FFI nativa nem de ausência geral de vazamentos.

## Verificação registrada

Em 25/10/2026, Linux x86-64, Rust/Cargo 1.98.1, GCC 16.2.1 e Python 3.14.8,
a suíte oficial passou layout, símbolos, três pares unsafe/sem unsafe,
cliente Rust → helper C e cliente C com 512 transformações, precedência e poison.
`nm` e `readelf` permitem reproduzir a inspeção do binário montado pelo harness.

O sandbox impediu a sonda independente do LeakSanitizer; a repetição autorizada
fora dele passou a suíte completa, inclusive a sonda de vazamento independente
de 37 bytes e a destruição normal e após poison. A supressão usada pelo harness
é específica do payload estático do runtime; o teste de distribuição não usa
supressão. Diagnósticos de panic controlado são esperados durante esses testes.
