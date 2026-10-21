# Referência C11

`make` compila; `make test` executa os vetores públicos do checkout local. `make check` também confere a revisão publicada no lock.

O algoritmo está em `implementation.c`; `api.h` e `adapter.c` reproduzem os materiais fornecidos. A saída é determinística e bytes binários aparecem em hexadecimal.

## Propriedade e linha do tempo

No starter, `produce` cria uma região com malloc e copia os quatro bytes para ela. Ainda dentro do produtor, free encerra o tempo de vida dessa região. A função devolve o endereço antigo; main tenta ler `p[0]` depois da liberação. Essa sequência — criação → liberação prematura → leitura — explica o relatório heap-use-after-free, independentemente do byte que uma execução sem sanitizador pareça imprimir.

Na correção, `copy_bytes` cria uma cópia independente e a devolve ainda viva. A origem permanece pertencendo ao chamador e não é liberada nem modificada pela função. O chamador passa a possuir a cópia: o adaptador a lê para imprimir os bytes e só então chama free. Para comprimento zero, a função também devolve uma alocação liberável. Se malloc falhar, retorna NULL e o adaptador reporta erro em stderr com status não zero.

`make sanitize` verifica a API e os vetores sob ASan/UBSan. A regressão que distingue as versões é `python3 ../../../challenges/016-use-after-free/tests/sanitizer_regression.py implementation.c`: ela exige o diagnóstico no starter e sucesso da correção.
