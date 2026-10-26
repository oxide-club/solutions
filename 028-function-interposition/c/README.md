# 028-function-interposition — referência em C

O objetivo é estudar interposição ELF sem alterar os resultados da função
observada. O público deve conhecer C, ABI, bibliotecas compartilhadas,
ponteiros para função, errno e threads. O escopo é a função própria
`calc_div`; o código não intercepta funções gerais da libc.

## Compilar, usar e verificar

Requer Linux x86-64 com glibc, compilador C11, pthreads, libdl, Make,
Python 3.10 ou superior e binutils para o mapa de símbolos.

```sh
make
./build/app > build/baseline.txt
LD_PRELOAD="$PWD/build/libtrace.so" ./build/app > build/traced.txt 2> build/events.jsonl
cmp build/baseline.txt build/traced.txt
LD_PRELOAD="$PWD/build/libtrace.so" ./build/app reenter > /dev/null
LD_PRELOAD="$PWD/build/libtrace.so" ./build/direct
make symbols
make test
make check
```

`make test` executa a suíte pública no checkout vizinho e compara a
aplicação entregue nos quatro modos de sink. `make check` também exige
a revisão e a limpeza do checkout conforme o
[guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

1. `libcalc.so` aplica a precedência NULL, divisor zero, overflow. Somente
   o sucesso escreve o resultado; a divisão inteira trunca para zero.
2. `libtrace.so` exporta o mesmo símbolo. Na primeira chamada,
   `pthread_once` resolve a definição posterior com `RTLD_NEXT` e depois
   resolve o callback exportado pela aplicação. Todos os workers esperam
   essa inicialização antes de usar os ponteiros publicados.
3. O wrapper salva errno de entrada, restaura esse valor após a resolução,
   delega exatamente uma vez e captura imediatamente errno de saída.
4. Uma chamada externa recebe ID atômico e TID Linux e produz uma linha
   JSON completa. O callback pode mudar errno ou falhar; o wrapper restaura
   o errno capturado antes de retornar o resultado original.
5. A guarda local de thread envolve formatação e callback. Uma chamada
   reentrante delega à função real e não gera novo evento.
6. `app` executa os seis cenários em quatro threads. Cada worker acumula
   seus resultados; a escrita de blocos iguais torna stdout determinístico
   sem serializar as chamadas observadas. O sink serializa cada linha de
   stderr com mutex. No modo `reenter`, chama a função uma vez por thread,
   antes de adquirir esse mutex.

## Decisões

`dlsym` é acompanhado por limpeza e consulta de `dlerror`; um endereço
nulo sem erro também é rejeitado porque não é chamável. Falha de resolução
encerra com diagnóstico `libtrace` e status 125. Ela não vira um erro
aritmético da API. A conversão de ponteiros usa a ABI POSIX e `memcpy`,
com verificação estática dos tamanhos, evitando aliasing por ponteiro
para tipo incompatível.

Há um único ponto de delegação antes do logging. A função real recebe os
argumentos originais em todos os caminhos, inclusive erros e reentrancy.
Somente retorno zero permite ler `*out` para o evento; os erros registram
`null`. O contador atômico usa ordem relaxada porque exige unicidade, sem
publicar outros dados. Os ponteiros usam a sincronização de `pthread_once`.
O callback vive no executável até o término; o exemplo não descarrega
bibliotecas durante chamadas.

O sink recebe uma linha por chamada e oferece a serialização exigida pelo
contrato. A falha é best effort: o evento pode ser perdido, sem modificar
retorno, saída ou errno. Nenhum lock do wrapper atravessa o callback.

## Mapa de símbolos e verificação

`make symbols` registra tabelas em `build/symbols.txt` e disassembly em
`build/direct-disassembly.txt`. Observe:

| Artefato | `calc_div` | Resolução |
| --- | --- | --- |
| `libcalc.so` | Definido, global | Função aritmética real. |
| `libtrace.so` | Definido, global | Wrapper pré-carregado. |
| `app` | `UND` | Busca dinâmica; o preload encontra o wrapper primeiro. |
| `direct` | Definido | Chamada incorporada no executável. |

Em `direct`, o disassembly mostra a chamada para a definição no próprio
executável. Preload não altera essa referência já vinculada. Não se usa
LTO: a intenção é manter a chamada examinável.

O controle `compare.py` exige stdout idêntico nos modos `healthy`,
`errno`, `fail` e `reenter`, 240 registros quando o sink funciona e nenhum
em `fail`. A suíte oficial acrescenta 4006 chamadas com cinco TIDs,
substituto opaco que detecta delegação omitida/duplicada, resolução ausente
e controle direto. Esses testes complementam a análise de publicação e
reentrancy; não provam ausência universal de data races.

## Limites

Custo O(1) e buffer de 512 bytes por chamada; o sink pode bloquear. Há
contenção no contador e na escrita do sink. A referência não promete
equivalência temporal, ordem global, uso em signal handlers, cancelamento
de threads, fork concorrente ou logging durante encerramento abrupto.
O overflow do ID encerra a instrumentação em vez de reutilizar IDs.
O cliente deve fornecer `ox_trace_sink` e ponteiros válidos conforme a API.
O header `calc.h` é cópia exata do material oficial, MIT.
