# 004-hexdump — referência em C11

Uma implementação possível está em [main.c](main.c). Tente o desafio antes de consultar a referência.

```sh
make
make test
make check
make clean
```

Execute estes comandos neste diretório. Binário: `build/oxide-hexdump`. `CHALLENGES_DIR` pode apontar para outro checkout do repositório challenges. `make test` verifica o checkout local; `make check` primeiro exige a revisão fixada no manifesto, preservando a proteção de manutenção. Não atualize o lock para contornar diferenças locais.

Formatação em blocos de 16 e coluna ASCII definida sem locale.

O contrato integral, formatos, limites e exemplos estão em `004-hexdump/index.md` no repositório challenges. A implementação verifica erros de leitura e escrita e ignora SIGPIPE quando disponível, permitindo emitir um diagnóstico. Não garante rollback da saída em caso de falha. Os helpers PPM fornecidos pelo desafio são dependências explícitas de compilação para os exercícios de imagens.
