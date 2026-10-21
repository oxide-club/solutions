# Referência C11: formato inferido

`make`, `make test`, `make check` (com guarda da revisão), `make clean`.

Hipótese inicial: os dois campos de dimensão usam little endian. A consulta de uma imagem 1×2 gera bytes 00 01 00 02, refutando essa hipótese; a interpretação big endian explica ambos. black/red/green/blue isolam os componentes: vermelho muda o terceiro byte de cada pixel, azul o primeiro, identificando BGR. labels 2×2 e rect 3×2 distinguem ordem de linhas de ordem de colunas: os pixels armazenados avançam primeiro em y. O cabeçalho tem PX, versão 1, reservado zero e dois u16 big endian. O payload é BGR em colunas. O leitor valida dimensões 1..64, versão e tamanho exato antes de exportar RGB em linhas.

Validação: fresh 2×3 usa cores não presentes no corpus. Rejeições cobrem versão, dimensões zero, truncamento e bytes finais. Consulte novas imagens via oracle.py para testar sua própria hipótese. Memória O(n), tempo O(w*h).
