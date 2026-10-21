# 007-image-bug — referência em C11

Uma implementação possível está em [main.c](main.c). Tente o desafio antes de consultar a referência.

```sh
make
make test
make check
make clean
```

Execute estes comandos neste diretório. Binário: `build/oxide-image-bug`. `CHALLENGES_DIR` pode apontar para outro checkout do repositório challenges. `make test` verifica o checkout local; `make check` primeiro exige a revisão fixada no manifesto, preservando a proteção de manutenção. Não atualize o lock para contornar diferenças locais.

O patch usa y×largura+x. O original usa altura e módulo para manter acessos válidos.

O contrato integral, formatos, limites e exemplos estão em `007-image-bug/index.md` no repositório challenges. A implementação verifica erros de leitura e escrita e ignora SIGPIPE quando disponível, permitindo emitir um diagnóstico. Não garante rollback da saída em caso de falha. Os helpers PPM fornecidos pelo desafio são dependências explícitas de compilação para os exercícios de imagens.
