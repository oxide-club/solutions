# 031-ipc-snapshots — referência em C mantida pelo Oxide Club

Esta referência implementa uma sessão cooperativa entre dois processos locais.
O objetivo é estudar como autenticação, transferência de descritores e exclusão
mútua se combinam. O nível é avançado: pressupõe processos, descritores, sockets,
memória virtual, ponteiros, pthreads e data races em C.

O ambiente é Linux x86-64, com a mesma glibc e ABI nos dois executáveis. Use C11,
pthreads, `memfd_create`, `getrandom` e `pthread_mutex_clocklock`; não há
dependências externas. O [modelo de ameaças](THREATS.md) delimita a confiança
nos participantes e acompanha a implementação.

## Como compilar e testar

Com `challenges` e `solutions` lado a lado, neste diretório:

```sh
make
./build/target --observer "$(pwd)/build/observer"
./build/target --observer "$(pwd)/build/observer" --test-worker
make test
make check
make clean
```

`make test` usa os testes públicos atuais. `make check` exige também a revisão
fixada em `solutions/challenges.lock.json`; consulte o
[guia de manutenção](../../docs/MAINTENANCE.md). `build/target-test` é o build
separado com `OX_TEST_HOOKS`: só ele aceita `--test-sequence-max`.

O observador executa snapshot, atribuição de `7`, snapshot, outra atribuição de
`7`, snapshot e STOP. Entre as atribuições e as observações seguintes, aguarda
20 ms para abrir espaço à worker, sem depender do seu escalonamento. Cada linha
JSON registra sequência, geração da resposta, geração da observação posterior
e contador respondido. Sem worker, este é um exemplo de saída:

```json
{"seq":1,"response_generation":0,"later_generation":0,"counter":0}
{"seq":2,"response_generation":1,"later_generation":1,"counter":7}
{"seq":3,"response_generation":1,"later_generation":1,"counter":7}
{"seq":4,"response_generation":2,"later_generation":2,"counter":7}
{"seq":5,"response_generation":2,"later_generation":2,"counter":7}
```

Com worker, os valores de geração e dos snapshots dependem do escalonamento.
Tokens e bytes do mutex nunca fazem parte dos logs.

## Como funciona

1. [target.c](target.c) cria o diretório privado, o socket e um `memfd` do
   tamanho de `struct ox_region`. Inicializa os dados e o mutex robusto entre
   processos antes de publicar qualquer descritor. Gera 32 bytes aleatórios,
   inicia o observador e entrega esses bytes pelo pipe herdado.
2. O alvo recebe uma conexão, confere `SO_PEERCRED` e lê HELLO. Tentativas de
   autenticação recusadas não recebem memória nem alteram o contador. Depois
   de autenticar, envia `OK 0 HELLO` com um descritor por `SCM_RIGHTS`.
3. [observer.c](observer.c) recebe o controle com `recvmsg` desde o primeiro
   byte. Fecha descritores excedentes, rejeita truncamento, exige exatamente
   um descritor e confere o tamanho com `fstat` antes de mapear. Adquire o mutex
   e confere a versão antes de copiar os demais campos.
4. O alvo valida o framing e a sequência antes de executar o comando. Erros
   COMMAND e VALUE consomem a sequência e preservam os dados. FRAME, SEQ e os
   demais erros fatais encerram a conexão. A sequência máxima só admite STOP.
5. SNAPSHOT e SET_COUNTER adquirem o mutex. O segundo confere overflow antes
   de alterar qualquer campo; atualiza contador, redundância e geração sob a
   mesma exclusão mútua. A resposta usa uma cópia privada feita dentro dessa
   seção crítica; escrever no socket não mantém o mutex ocupado.
6. Depois de cada resposta STATE, o observador faz outra cópia sob mutex.
   Confere geração posterior maior ou igual à respondida, invariantes e dados
   iguais quando as gerações coincidem. A geração não autoriza leituras sem
   mutex: o programa não implementa seqlock.
7. STOP encerra e aguarda a worker antes de responder. Nos outros caminhos,
   o cleanup também interrompe e aguarda a worker. O alvo fecha a conexão,
   aguarda o observador e só então libera os recursos compartilhados.

## Decisões

[common.c](common.c) concentra inteiros canônicos, framing, transferência de
descritores e prazos. As operações de transporte usam descritores não
bloqueantes e `poll`, com deadline monotônico calculado uma única vez por
operação. EINTR e I/O parcial preservam esse deadline. A aquisição do mutex usa
o mesmo relógio e o deadline restante. Após um timeout, uma tentativa limitada
de enviar o diagnóstico recebe prazo próprio; isso não reinicia a operação que
falhou.

A leitura de linhas usa um byte por chamada. Isso torna explícito o limite de
256 bytes incluindo LF e evita consumir bytes do próximo pedido. No HELLO,
também permite associar os descritores aos primeiros bytes sem presumir que
uma escrita corresponda a uma leitura. O custo é maior número de syscalls.
Um envio parcial de HELLO continua com escrita comum: nunca repete SCM_RIGHTS.

A worker usa o mesmo mutex dos dois processos para todos os campos da região.
Seus sinais de parada e falha usam atômicos locais ao alvo. Cada atualização
confere geração antes de modificar campos. Uma falha invalida a sessão, em vez
de tentar reconstruir dados possivelmente inconsistentes.

`EOWNERDEAD` entrega ownership, mas não garante consistência. `ox_lock` libera
esse ownership sem chamar `pthread_mutex_consistent`; o chamador encerra a
sessão. No cleanup, um mutex irrecuperável sem usuários é descartado com o
mapeamento. Um mutex saudável só é destruído depois de adquirido e liberado,
quando worker e observador já terminaram.

## Recursos e sincronização

| Recurso | Criação e transferência | Encerramento |
| --- | --- | --- |
| Diretório e socket | Alvo; modos 0700 e 0600 | Alvo fecha listener, remove socket e diretório |
| Pipe do token | Alvo; só a leitura atravessa `exec` | Pai fecha leitura e depois escrita; observador fecha leitura depois dos 32 bytes |
| Conexão | Alvo aceita; observador conecta | Cada processo fecha seu descritor em sucesso ou falha |
| `memfd` | Alvo dimensiona; SCM_RIGHTS entrega outra referência | Cada processo fecha seu descritor; observador fecha também os excedentes |
| Mapeamento | Cada processo chama `mmap` | Observador desmonta antes de sair; alvo aguarda usuários antes de desmontar |
| Mutex | Alvo inicializa antes da publicação | Alvo destrói somente o mutex saudável sem usuários; descarta o irrecuperável |
| Worker e filho | Alvo inicia ambos | Alvo faz `join` e `waitpid`; encerra o filho que ultrapassa o prazo e faz `waitpid` |

A inicialização ocorre antes da publicação, quando só o alvo acessa os dados.
Depois dela, as únicas leituras dos campos são `ox_valid` e `ox_copy`, chamadas
com ownership do mutex. As únicas escritas dos campos estão em SET_COUNTER e
na worker, também com ownership. Geração, contador e redundância são
atualizados juntos; nenhuma cópia parcial atravessa a seção crítica. Um mutex
ocupado que não possa ser adquirido no prazo não é destruído.

O descritor recebido pelo observador continua válido depois que o alvo fecha
o seu; o mapeamento continua válido depois que o observador fecha o descritor.
Por isso, o alvo aguarda os usuários antes de destruir o mutex, em vez de
deduzir seu lifetime apenas dos descritores locais.

## Evidências

Hipótese: a redundância e a monotonicidade se preservam se todas as cópias e
atualizações usarem o mesmo mutex. Os testes públicos executam mil snapshots
com worker e comparam resposta e leitura posterior. Isso complementa a revisão
dos acessos acima; não prova ausência de todos os defeitos de sincronização.

A validação local usa `-Wall -Wextra -Wpedantic -Werror`. A suíte verifica
autenticação recusada seguida de nova tentativa, fragmentação, comandos,
sequências, limites de linha, geração máxima, morte do dono do mutex, peer
desconectado e prazos de dois segundos. Executa também dez sessões sucessivas
por build e verifica ausência de sockets e diretórios abandonados. A suíte
do observador oferece seis HELLO inválidos, com auditoria de fechamento de
descritores. Os dois controles positivos usam o alvo e observador reais.

Resultado observado: `make test` passou com o checkout local. `make check`
também passou com um checkout limpo da revisão
`3501abf17d03d8c268ae4fda0516d511545f6bf1`: 22 sessões no build normal,
24 no build com hooks, seis recusas do observador e dois controles positivos.
O build normal recusou `--test-sequence-max` com status 2. A comparação de
`region.h` com o material oficial, a formatação C e os links locais passaram.

Ambiente observado: Linux 7.2.8-9-omarchy, glibc 2.44, GCC 16.2.1, x86-64. A execução de
IPC exige permissão para sockets Unix e memória compartilhada; no sandbox
local, o setup foi recusado. Os testes de sessão foram executados fora dele.

## Limites

O layout de [region.h](region.h) é cópia do material oficial e depende de ABI,
libc e arquitetura iguais. Não serve como formato persistente nem protocolo
entre máquinas. A sessão tem um observador autenticado e contador limitado a
1.000.000; não aceita endereços ou código arbitrário.

Estado e mensagens ocupam espaço constante. Cada pedido usa memória O(1) e
trabalho O(n) para n bytes da linha, limitado a 256; agendamento e espera pelo
mutex determinam a latência observada. Os prazos limitam esperas cooperativas,
mas não fornecem garantias de tempo real sob escalonamento hostil.

Os testes não isolam um observador malicioso do mesmo UID. A capacidade de
escrita inclui o mutex e todos os campos; consulte as consequências no
[modelo de ameaças](THREATS.md). A auditoria estática de ownership e cleanup
complementa os testes; ausência de arquivos residuais sozinha não prova
ausência de vazamento de todos os recursos.
