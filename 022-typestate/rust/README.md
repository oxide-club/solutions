# 022-typestate — referência em Rust

Esta biblioteca representa um protocolo por tipos. O objetivo é estudar como
ownership e encapsulamento impedem sequências inválidas e preservam o recurso
quando uma operação válida falha. O nível é intermediário; é preciso conhecer
`struct`, generics, `Result`, referências e movimentação de valores em Rust.
O escopo é um buffer em memória, sem I/O real.

## Compilar, executar e testar

Requer Rust/Cargo estável, edição 2021, Python 3 e um checkout de `challenges`
ao lado de `solutions`. Não usa crates externas.

```sh
make
./build/library/debug/examples/flow
make test
make format
make check
```

O exemplo [flow.rs](examples/flow.rs) escreve `abcd`, recupera uma falha de
finalização e confere os bytes com `assert_eq!`. No sucesso, encerra sem saída.
`make test` copia código e testes oficiais para `build/check`, incluindo a
fixture de bytes; não altera os enunciados. Executa testes funcionais e pares
de clientes válidos/inválidos. `make check` exige também a revisão registrada
em `challenges.lock.json`; para alterações locais, use `make test`.
`CHALLENGES_DIR=/caminho/absoluto/challenges` seleciona outro checkout.

## Como funciona

1. `new` cria um vetor vazio e retém o plano de falhas independentes.
2. `open` consome `Resource<Closed>`. No sucesso, move os campos para um
   `Resource<Open>`; no erro, devolve o recurso fechado com o gatilho consumido.
3. `write` verifica o espaço restante antes de consumir o gatilho. Escrita
   vazia retorna sucesso. Escrita elegível acrescenta todos os bytes em ordem.
4. `finalize` faz a mesma transferência para `Resource<Finalized>` ou devolve
   o recurso aberto. `bytes` oferece leitura somente no estado finalizado.

| Estado | Método | Resultado | Efeito de falha |
| --- | --- | --- | --- |
| `Closed` | `open(self)` | `Resource<Open>` | Devolve `Resource<Closed>` |
| `Open` | `write(&mut self, bytes)` | `()` | Preserva bytes e estado |
| `Open` | `finalize(self)` | `Resource<Finalized>` | Devolve `Resource<Open>` |
| `Finalized` | `bytes(&self)` | `&[u8]` | Não falha |

## Decisões

Os `impl` especializados expõem apenas operações válidas. `PhantomData<S>`
expressa a relação com o estado sem guardar um marcador em runtime; também
faz o compilador considerar `S` nos auto traits. Os marcadores usados não
possuem recursos. Os campos privados e a ausência de construtores públicos
para estados intermediários impedem fabricar um recurso aberto ou finalizado.
Não há `Clone`/`Copy` de recursos, transição inversa ou flag de estado.

A segunda finalização e o uso após `open` são rejeitados por movimentação de
ownership; escrita antes de abrir ou após finalizar falha por inexistência do
método. Os pares de compilação conferem os códigos correspondentes, com um
controle válido antes de cada rejeição. O erro de transição contém ownership
do recurso original; uma nova tentativa usa os mesmos dados.

O invariante `data.len() <= 4096` permite calcular `4096 - data.len()` sem
underflow. Comparar o comprimento recebido com esse espaço evita somar
comprimentos potencialmente grandes. A ordem das verificações preserva o
gatilho de escrita quando falta capacidade ou quando a entrada é vazia.

## Limites

Escrever n bytes custa O(n), além de eventual realocação do vetor; transições
movem seus campos em O(1). O buffer guarda no máximo 4096 bytes lógicos.
Falhas de alocação que abortam o processo não se tornam `WriteError`.
O drop de um recurso aberto abandona seus bytes, sem finalizar nem persistir.
Typestate restringe a sequência de operações; falhas operacionais continuam
dinâmicas. O núcleo proíbe `unsafe` por atributo do compilador.

## Verificação registrada

Em 25/10/2026, Linux x86-64, Rust/Cargo 1.98.1 e Python 3.14.8:
`make test` passou os quatro testes de contrato e os pares de compilação.
O starter foi executado antes da implementação e falhou nos quatro testes
funcionais por `todo!`. Esses resultados cobrem os casos executados e não
substituem a revisão do encapsulamento descrita acima.
