# 006-stream-search — referência em C11

Uma implementação possível está em [main.c](main.c). Tente o desafio antes de consultar a referência.

```sh
make
make test
make check
make clean
```

Execute estes comandos neste diretório. Binário: `build/oxide-stream-search`. `CHALLENGES_DIR` pode apontar para outro checkout do repositório challenges. `make test` verifica o checkout local; `make check` primeiro exige a revisão fixada no manifesto, preservando a proteção de manutenção. Não atualize o lock para contornar diferenças locais.

KMP preserva estado entre blocos de 4096 bytes; O(n+m) tempo e O(m+4096) memória.

O contrato integral, formatos, limites e exemplos estão em `006-stream-search/index.md` no repositório challenges. A implementação verifica erros de leitura e escrita e ignora SIGPIPE quando disponível, permitindo emitir um diagnóstico. Não garante rollback da saída em caso de falha. Os helpers PPM fornecidos pelo desafio são dependências explícitas de compilação para os exercícios de imagens.
