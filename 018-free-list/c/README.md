# 018-free-list — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [implementation.c](implementation.c).
Recomendamos tentar o desafio antes de consultá-la. A definição, os
materiais e os testes públicos ficam em `018-free-list/` no repositório
separado `challenges`.

Os arquivos [api.h](api.h) e [adapter.c](adapter.c) são cópias exatas dos
materiais do desafio. Não os edite: `tools/verify_local.py` confere que as
cópias continuam idênticas. Por isso, eles mantêm a formatação original.

Neste diretório (`solutions/018-free-list/c`), com compilador C11:

```sh
make
printf '24\na 8\na 8\na 8\nm\nf 1\na 16\nm\nf 0\na 16\nm\n' | ./build/oxide-free-list
# 0:0
# 1:8
# 2:16
# 0:8:U 8:8:U 16:8:U
# fail
# 0:8:U 8:8:F 16:8:U
# 3:0
# 0:16:U 16:8:U
make check
```

`make test` compila também `tests/api_test.c` do desafio contra a
implementação e executa esse teste direto antes dos vetores públicos.
Durante alterações conjuntas dos repositórios, use `make test` para
executar os testes locais sem exigir a revisão fixada no manifesto.
Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

O mapa é um array de descritores `Block`, cada um com offset, tamanho e a
marca `used`. Os descritores ficam em `Pool.blocks`, fora da região, em
ordem de offset.

- `pool_init` arredonda a capacidade para baixo a um múltiplo de 8 e cria
  um único bloco livre com esse tamanho. Capacidade útil zero deixa o
  mapa vazio.
- `pool_alloc` arredonda o pedido para cima a um múltiplo de 8. Percorre
  o mapa em ordem e escolhe o primeiro bloco livre com tamanho
  suficiente. Se o bloco é maior que o pedido, divide: o primeiro pedaço
  fica usado e o segundo fica livre. Marca o bloco e devolve
  `base + offset`.
- `pool_free` localiza o bloco pelo offset, marca como livre e une todos
  os pares de blocos livres vizinhos.

## Fragmentação e coalescência

No exemplo acima, três blocos de 8 bytes preenchem a região de 24. Liberar
só o bloco central deixa 8 bytes livres entre dois blocos usados. Um
pedido de 16 falha: a soma dos espaços livres bastaria, mas nenhum bloco
livre contíguo tem 16 bytes. Isso é fragmentação externa.

Liberar o primeiro bloco cria dois blocos livres vizinhos, `0:8:F` e
`8:8:F`. A coalescência os une em `0:16:F`. O mesmo pedido de 16 agora
funciona no offset 0.

## Invariantes

- Os blocos cobrem a capacidade útil em ordem, sem lacunas nem
  sobreposição. A soma dos tamanhos é igual à capacidade útil.
- Todo offset e todo tamanho são múltiplos de 8. Todo tamanho é positivo.
- Depois de `pool_free`, dois blocos livres nunca são vizinhos.
- Uma operação que falha não altera o mapa. `pool_alloc` confere a vaga
  para o novo descritor antes de modificar qualquer bloco.

## Decisões

A divisão de um bloco cria um descritor novo. Quando o array já tem 1024
descritores, a divisão falha e o mapa fica intacto. Um encaixe exato não
precisa de descritor novo e funciona mesmo com o array cheio. O caso de
teste com 1023 reservas de 8 bytes em uma região de 8200 verifica os dois
comportamentos.

A verificação `n > SIZE_MAX - 7` vem antes do arredondamento. Sem ela,
`n + 7` poderia transbordar e virar um pedido pequeno, como no comando
`o`.

A coalescência faz um único percurso do mapa. O bloco recém-liberado é o
único que pode ter criado vizinhos livres, e o percurso une os dois lados.
Os deslocamentos do array custam O(count), aceitável para 1024
descritores.

## Limites

Alocação e liberação custam O(count) pelo percurso e pelos deslocamentos.
O pool não detecta liberações inválidas nem implementa `realloc` ou
best-fit; o enunciado lista essas operações como extensões.
