# 029-minimal-debugger — referência em C mantida pelo Oxide Club

[oxdbg.c](oxdbg.c) implementa um controlador de um filho e uma thread em
Linux x86-64. O objetivo é estudar estados, paradas de `ptrace`, signal delivery
e child reaping. A leitura exige C, processos, sinais, registradores x86-64 e
noções de assembly. Esta é uma abordagem possível para o contrato do desafio.

## Compilar e executar

Use GCC ou Clang, glibc, binutils, Make e Python 3. Mantenha o checkout oficial
de `challenges` ao lado de `solutions`. A partir deste diretório:

```sh
make
make fixtures
printf 'run\nstep\nregs\nstep\nregs\nstep\nregs\ncontinue\n' |
    ./build/oxide-minimal-debugger -- ../../../challenges/029-minimal-debugger/build/arith-exec
make test
```

`make fixtures` usa as receitas oficiais para compilar os alvos ET_EXEC/PIE e
os supervisores. `make test` executa as suítes funcional, de ciclo de vida e de
indisponibilidade do desafio. Não modifique permissões do sistema para contornar
uma recusa de `ptrace`: execute os testes em um ambiente próprio autorizado.

`make check` também exige o checkout limpo e a revisão de `challenges` registrada
em `solutions/challenges.lock.json`. Durante o desenvolvimento conjunto, use
`make test`; veja o [guia de manutenção](../../docs/MAINTENANCE.md).
`CHALLENGES_DIR=/caminho/absoluto/challenges` seleciona outro checkout.
`make clean` remove apenas o executável local.

## Como funciona

1. `main` lê uma linha de comando inteira e confere o estado antes de agir.
   `run` só existe em `NEW`; não há attach por PID nem reinício.
2. `start` cria um pipe e um filho. O filho chama `PTRACE_TRACEME`, comunica
   uma eventual falha pelo pipe e faz a parada inicial com `SIGSTOP`.
3. O pai registra essa parada como `sync`, consome o sinal e configura `TRACEEXEC`, `EXITKILL` e
   `TRACESYSGOOD`. O filho então executa o alvo; o descritor de escrita fecha
   automaticamente em um exec bem-sucedido, por `O_CLOEXEC`.
4. O evento exec é registrado separadamente. Uma retomada com `PTRACE_SYSCALL`
   drena a saída de `execve`, reconhecida por `SIGTRAP | 0x80`. Essa parada
   interna aparece como `exec_boundary`. Só então `run` devolve o controle.
5. `resume` retoma com `PTRACE_SINGLESTEP` ou `PTRACE_CONT`, entrega o sinal
   pendente uma única vez e aguarda o próximo evento. `observe` interpreta
   status, evento e `siginfo` antes de escolher a causa da parada.
6. `registers` consulta `PTRACE_GETREGS` somente em `STOPPED`. Cada evento é
   uma linha JSON completa, com sequência consecutiva e flush imediato.
7. `quit`, EOF e falhas levam a `cleanup`: o pai envia `SIGKILL` ao filho
   ainda existente e repete `waitpid` até recolhê-lo. `EINTR` apenas reinicia
   a espera. Se o tracer desaparecer, `EXITKILL` impede que o tracee continue.

A ferramenta retorna 0 para encerramento controlado, mesmo se o alvo terminou
por sinal; o status do alvo fica no evento `exit`. Uso inválido retorna 2.
Timeout, erro de exec, parada não suportada e outras falhas retornam 1.

## Estados e predicados

| Estado | Operação ou observação válida | Próximo estado |
| --- | --- | --- |
| `NEW` | `run` cria o único filho | `STARTING` |
| `STARTING` | sincronização, opções e retomada | `RUNNING` |
| `RUNNING` | exec ou parada reconhecida | `STOPPED` |
| `STOPPED` | `regs` lê registradores | `STOPPED` |
| `STOPPED` | `step` ou `continue` retoma | `RUNNING` |
| `RUNNING` | status de término de `waitpid` | `EXITED` |
| Qualquer estado ativo | falha ou timeout | `FAILED`, seguido de cleanup |
| Qualquer estado | comando inadequado | mesmo estado, evento `rejected` |

`quit` e EOF encerram a sessão e recolhem o filho quando necessário. A
rejeição de comandos não chama `ptrace`, não cria filho e não consome sinal
pendente. Depois de `EXITED` não há novo `run`.

## Decisões

### Classificação de paradas e sinais

A ordem de classificação evita usar o número `SIGTRAP` como prova de stepping:

- `PTRACE_EVENT_EXEC` com `SIGTRAP` é um evento do tracer.
- Outros eventos ou uma parada sem `siginfo` recebem `unsupported_stop`.
  Isso inclui group-stop, para o qual `GETSIGINFO` não entrega a informação
  de um signal-delivery-stop.
- Um step pendente com `TRAP_TRACE` recebe `step` e não deixa sinal pendente.
- Um step que acabou de injetar um sinal, seguido de `SIGTRAP` com
  `si_code == SIGTRAP`, recebe `signal_step`. É a parada interna descrita
  no guia local; o sinal já foi entregue e não deve ser repetido.
- Códigos não positivos, `SI_KERNEL` e `SIGTRAP` com `TRAP_BRKPT` recebem
  `signal_delivery`. Isso preserva tanto `raise(SIGTRAP)` quanto `int3`,
  inclusive quando surgem durante um pedido de step.
- Outros códigos positivos ficam fora do núcleo e encerram a sessão com
  `unsupported_stop`.

O sinal pendente é entregue como argumento da próxima retomada. Depois de
uma retomada bem-sucedida o campo volta a zero. Eventos de exec, paradas de
syscall e traps internos não viram sinais a reinjetar. O `SIGSTOP` inicial
pertence apenas ao protocolo; um `SIGSTOP` posterior passa pelo tratamento
de sinais e o group-stop subsequente encerra a sessão como não suportada.

### Fronteira de exec e endereços

No ambiente observado, um step imediatamente após o evento exec pode parar
na saída da syscall, antes de executar a primeira instrução. Drenar essa
fronteira com `PTRACE_SYSCALL` evita contar esse trap como execução aritmética.
A parada com bit `0x80` só é aceita nessa fase conhecida. Uma observação
inesperada gera erro e cleanup, em vez de suprimir traps arbitrariamente.

O controlador não calcula load base. A interpretação dos snapshots usa os
símbolos do próprio alvo e os tamanhos das instruções obtidos por `objdump`.
Na versão ET_EXEC observada, `_start` é `0x401000`. No PIE a base pode mudar;
as relações `RIP - entrada = 5`, `8` e `11` e os valores aritméticos devem
permanecer. O início do primeiro mapeamento não é usado como base universal.

### Falhas, prazos e recursos

O pipe transporta um registro pequeno com a operação e o errno obtidos pelo
filho. Isso distingue falha real de `PTRACE_TRACEME` de erro de exec. O pai
fecha sua ponta de escrita imediatamente; ambos fecham as pontas de leitura
que não usam. O pai fecha sua leitura em todos os caminhos de `start`.

Uma negação de TRACEME com `EPERM`, `EACCES` ou ausência de implementação com
`ENOSYS` gera status 77 e evento `unavailable`, incluindo PID real, operação,
errno e o testemunho `child_ptrace_result`. A ferramenta registra também o
diagnóstico em stderr. Um erro funcional posterior não se converte em 77.
A suíte compara essa informação com uma tentativa observada por supervisor
independente; observar seccomp ativo, isoladamente, não caracteriza bloqueio.

Cada retomada usa um prazo monotônico de cinco segundos. `waitpid(WNOHANG)`
e pausas de um milissegundo permitem conferir o prazo sem handlers de alarme.
EINTR não renova o prazo. O caminho de cleanup usa `SIGKILL` e espera pelo
filho, inclusive quando havia uma parada pendente antes do sinal. O filho
só deixa de pertencer à sessão depois de um status de término recolhido.

## Hipóteses e evidências

Antes da execução, o disassembly do fixture permite prever três snapshots:

| Instrução executada | Predicado previsto | Observação em ET_EXEC |
| --- | --- | --- |
| `mov $5, %eax` | `RAX = 5`; RIP chega a `after_first` | `RAX=0x5`, `RIP=0x401005` |
| `add $3, %eax` | `RAX = 8`; RIP chega a `after_second` | `RAX=0x8`, `RIP=0x401008` |
| `mov %rax, %rbx` | `RBX = 8`; RIP chega a `after_third` | `RBX=0x8`, `RIP=0x40100b` |

O registro completo observado está em [session.jsonl](session.jsonl). Os PIDs,
RSP, endereços e flags desse arquivo pertencem àquela execução; não são
constantes do contrato. A transcrição termina com `exit_code: 7`, enquanto
a ferramenta termina com 0. Os três snapshots mostram alterações de estado
de máquina coerentes com as instruções, sem depender de endereços da stack.

Verificação local em 25/10/2026: Linux `7.2.8-9-omarchy`, x86-64,
GCC `16.2.1 20260810`, glibc `2.44`, `ptrace_scope=1`. O starter oficial falhou
com o diagnóstico de implementação ausente, antes da referência.
A compilação da referência usou C11, `-O2 -g -Wall -Wextra -Wpedantic -Werror`.
O sandbox bloqueou o probe independente com `PTRACE_TRACEME`, errno 1; nele,
a suíte retornou 77 e não executou os testes funcionais.

A mesma suíte passou fora do sandbox, em execução autorizada: ET_EXEC/PIE,
estados inválidos, três snapshots, sinais USR1/TRAP/TERM por step e continue,
trap pendente durante step, `int3`, group-stop, exec inválido, quit/EOF,
timeout, EINTR injetado, EXITKILL e child reaping. Passaram também o filtro
seccomp neutro e a negação comprovada por notificação do kernel, com
correspondência entre o evento `unavailable` e a tentativa real do filho.

## Limites

O núcleo acompanha um filho e uma thread em Linux x86-64/glibc. Não oferece
attach, breakpoints, edição de sinais, symbolication, reinício, tracing de
syscalls como interface pública nem controle de threads ou filhos adicionais.
Use apenas os alvos didáticos preparados. O pipe de diagnóstico pertence ao
protocolo interno e não é uma interface para programas arbitrários.

Não há garantia de identidade dos endereços entre execuções com ASLR. A
classificação de `signal_step` e o drain de exec seguem o ambiente Linux do
contrato; uma diferença de protocolo resulta em erro controlado.

Cada espera usa memória constante. A leitura de comandos usa memória
proporcional à maior linha recebida. Uma falha de saída pode deixar JSON
parcial; a sessão então termina e recolhe o filho. Os testes cobrem os casos
executados e complementam a revisão dos caminhos de recursos e sinais.
