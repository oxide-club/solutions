# 001-wc — referência mantida pelo Oxide Club

Uma possível solução comentada está disponível para estudo. Recomendamos
tentar o desafio antes de consultá-la, mas você pode usá-la quando achar apropriado.

O código está em [main.c](main.c). A definição e os testes públicos ficam
no diretório `001-wc/` do repositório separado `challenges`.

Na raiz de `solutions`:

```sh
make build/001-wc
printf 'Hello world\nC is fun\n' | ./build/001-wc
# 2 5 21
python3 ../challenges/001-wc/tests/test.py ./build/001-wc
```

## Decisões

`fgetc` permite processar a entrada incrementalmente. Seu resultado é
armazenado em `int` para preservar a distinção entre um byte e `EOF`.
Contamos uma palavra na transição de separador para não separador, inclusive
quando o arquivo termina sem uma quebra de linha.

A comparação explícita dos seis separadores segue o contrato ASCII e não
depende da locale. O uso de `unsigned long long` pressupõe, como a definição,
que as contagens caibam no tipo. Não há tratamento de overflow.

Para N bytes, o tempo é O(N) e o espaço adicional é O(1). O programa inclui
diagnósticos básicos de I/O, mas não implementa flags, arquivos por argumento
ou contagem de caracteres Unicode. Compare outras estratégias, como leitura
em blocos, sem confundir esta referência com a única solução possível.
