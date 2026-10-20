# 003-rle — referência em Rust mantida pelo Oxide Club

Uma possível solução comentada está em [main.rs](main.rs). Recomendamos tentar
o desafio antes de consultá-la. A definição e os testes públicos ficam em
`003-rle/` no repositório separado `challenges`.

Neste diretório (`solutions/003-rle/rust`), com Rust estável e Make:

```sh
make
printf 'AAAAABBBCC' > build/original
./build/oxide-rle encode < build/original > build/encoded
od -An -tx1 build/encoded
# 05 41 03 42 02 43
./build/oxide-rle decode < build/encoded > build/decoded
cmp build/original build/decoded
printf '\003' | ./build/oxide-rle decode
# Diagnóstico de par incompleto em stderr; status diferente de zero.
make check
```

O Makefile usa `rustc` diretamente, edição 2021, sem crates externos.
Durante alterações conjuntas, `make test` usa os testes locais sem exigir
a revisão fixada no manifesto. Consulte o [guia de manutenção](../../docs/MAINTENANCE.md).

## Decisões

`BufReader` e `BufWriter` mantêm buffers de tamanho constante. A leitura retorna
`Option<u8>` para distinguir EOF dos 256 valores válidos e repete operações
interrompidas. Nenhuma conversão de texto é aplicada aos dados.

O encode mantém valor e contagem, emitindo antes de ultrapassar 255.
O decode aceita pares adjacentes iguais e escreve blocos de até 255 bytes.
`write_all` trata escritas parciais; o flush explícito detecta erros que seriam
ignorados pelo descarte do buffer. Argumentos são lidos com `args_os` para
rejeitar inclusive argumentos não UTF-8 sem panic.

O encode custa O(n), o decode custa O(m + n), sendo m os bytes comprimidos
e n os bytes produzidos. Ambos usam O(1) memória adicional. Em erros, uma
saída parcial pode já ter sido escrita. RLE pode dobrar o tamanho de dados
sem repetições; esta referência não adiciona cabeçalho ou limite de expansão.
