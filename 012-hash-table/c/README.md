# 012-hash-table — referência em C mantida pelo Oxide Club

Uma possível solução comentada está em [implementation.c](implementation.c).
Recomendamos tentar o desafio antes de consultá-la. A definição, os
materiais e os testes públicos ficam em `012-hash-table/` no repositório
separado `challenges`.

Os arquivos [api.h](api.h) e [adapter.c](adapter.c) são cópias exatas dos
materiais do desafio. Não os edite: `tools/verify_local.py` confere que as
cópias continuam idênticas. Por isso, eles mantêm a formatação original.

Neste diretório (`solutions/012-hash-table/c`), com compilador C11:

```sh
make
printf 'p 0 10\np 8 20\np 16 30\np 24 40\ns\np 32 50\ns\ng 8\ng 1\n' | ./build/oxide-hash-table
# ok
# ok
# ok
# ok
# 4/8
# ok
# 5/16
# 20
# missing
make check
```

`make test` compila também `tests/api_test.c` do desafio contra a
implementação e executa esse teste direto antes dos vetores públicos.
Durante alterações conjuntas dos repositórios, use `make test` para
executar os testes locais sem exigir a revisão fixada no manifesto.
Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Como funciona

A tabela é um array de slots `Pair`. Cada slot guarda chave, valor e a
marca `occupied`. A posição inicial de uma chave é o hash
`(uint32_t)key & (capacity - 1)`.

- `find_slot` parte da posição inicial e avança um slot por vez até
  encontrar a chave ou um slot vazio. Esse percurso é a sondagem linear.
- `table_put` primeiro procura a chave. Se ela existe, só atualiza o
  valor. Se é nova, confere o fator de carga, cresce quando necessário e
  grava o par no slot devolvido por `find_slot`.
- `grow` aloca uma reserva com o dobro de slots, zera todos e reinsere
  cada par com o hash da nova capacidade.
- `table_get` usa `find_slot` e devolve o valor se o slot está ocupado.
- `table_destroy` libera a reserva e volta ao estado vazio.

## Colisões

As chaves 0, 8, 16 e 24 têm o mesmo hash na capacidade 8: todas caem no
slot 0. A sondagem linear coloca a segunda no slot 1, a terceira no slot 2
e a quarta no slot 3. Uma consulta percorre a mesma sequência e para na
chave procurada ou no primeiro slot vazio.

## Invariantes

- `capacity` é zero ou uma potência de dois. Por isso, `& (capacity - 1)`
  equivale ao módulo e nunca produz índice fora do array.
- `count <= capacity / 2` depois de cada inserção. A tabela cresce antes
  de inserir uma chave nova quando `count >= capacity / 2`. Com metade dos
  slots vazios, a sondagem sempre encontra um slot vazio e termina.
- Todo slot é inicializado com `memset` antes de qualquer consulta a
  `occupied`.
- Uma falha de alocação ou de overflow preserva pares, contagem e
  capacidade. `grow` só substitui a reserva depois do sucesso.

## Decisões

A verificação `count >= capacity / 2` acontece só para chaves novas.
Atualizar uma chave existente não conta nem aloca, como o enunciado exige.

`grow` reinsere os pares com `find_slot` na nova reserva. Copiar os slots
para os mesmos índices quebraria as consultas, porque o hash depende da
capacidade.

Os cálculos de capacidade verificam `SIZE_MAX / 2` e
`SIZE_MAX / sizeof(Pair)` antes da multiplicação. Assim, o tamanho em
bytes passado ao alocador nunca transborda.

## Limites

Com fator de carga até 1/2, inserção e consulta custam O(1) em média. O
crescimento custa O(capacity), mas o custo amortizado por inserção é
O(1). A tabela não suporta remoção; o enunciado lista essa operação como
extensão, com estados adicionais de slot.
