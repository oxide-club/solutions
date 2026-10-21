#!/bin/sh
# Reconstrói o alvo do desafio 015 e confere os offsets citados nos materiais.
#
# Os traces e os procedimentos de debugger em materials/traces.md citam
# endereços do ELF distribuído. Esses endereços dependem do compilador, do
# linker e das flags. Esta versão do alvo foi gerada com a toolchain abaixo e
# o resultado é idêntico byte a byte quando a mesma toolchain é usada.
set -eu
cd "$(dirname "$0")"

CC="${CC:-cc}"
EXPECTED_CC="GCC) 16.2.1 20260810"
EXPECTED_BINUTILS="2.47"
CFLAGS="-std=c11 -O0 -g0 -Wall -Wextra -Wpedantic -Werror"
EXPECTED_ENTRY=10a0
EXPECTED_MAIN=1288
EXPECTED_MAIN_END=135a

out=../../../challenges/015-mystery-program/materials
# Gere e confira os dois arquivos antes de tocar nos materiais distribuídos.
work=$(mktemp -d "$out/.rebuild-target.XXXXXX")
trap 'rm -rf "$work"' 0
trap 'exit 1' HUP INT TERM
bin="$work/mystery-linux-x86_64"
asm="$work/mystery.asm"

cc_version=$("$CC" --version | head -n 1)
objdump_version=$(objdump --version | head -n 1)
case "$cc_version" in
  *"$EXPECTED_CC"*) ;;
  *) echo "aviso: compilador diferente do registrado ($cc_version); esperado $EXPECTED_CC" >&2 ;;
esac
case "$objdump_version" in
  *"$EXPECTED_BINUTILS"*) ;;
  *) echo "aviso: binutils diferente do registrado ($objdump_version); esperado $EXPECTED_BINUTILS" >&2 ;;
esac

# shellcheck disable=SC2086
"$CC" $CFLAGS main.c -o "$bin"
objdump -d -Mintel "$bin" > "$work/disassembly.raw"
# Preserve o nome publicado no cabeçalho, sem incluir o caminho temporário.
awk -v source="$bin:" -v published="$out/mystery-linux-x86_64:" '
  index($0, source) == 1 { $0 = published substr($0, length(source) + 1) }
  { print }
' "$work/disassembly.raw" > "$asm"
strip "$bin"

entry=$(readelf -h "$bin" | sed -n 's/.*Entry point address: *0x//p')
main_start=$(sed -n 's/^0*\([0-9a-f]*\) <main>:$/\1/p' "$asm")
# Fim exclusivo de main, como em `disassemble INICIO,FIM` do GDB:
# endereço da última instrução mais o tamanho dela em bytes.
last_insn=$(awk -v start="$main_start" '
  found && /^[0-9a-f]+ <.*>:$/ { exit }
  found && /^ *[0-9a-f]+:\t/ { last = $0 }
  $0 ~ ("^0*" start " <main>:$") { found = 1 }
  END {
    split(last, f, "\t"); sub(/^ */, "", f[1]); sub(/:$/, "", f[1])
    n = split(f[2], bytes, " ")
    print f[1], n
  }' "$asm")
main_end=$(printf '%x' $(( 0x${last_insn% *} + ${last_insn#* } )))

status=0
check() {
  if [ "$2" != "$3" ]; then
    echo "offset alterado: $1 = 0x$2, materiais citam 0x$3" >&2
    status=1
  fi
}
check "entry point" "$entry" "$EXPECTED_ENTRY"
check "inicio de main" "$main_start" "$EXPECTED_MAIN"
check "fim de main" "$main_end" "$EXPECTED_MAIN_END"

if [ "$status" -ne 0 ]; then
  cat >&2 <<'MSG'
Os offsets do novo alvo diferem dos citados em materials/traces.md e no
enunciado. Antes de distribuir, atualize os endereços nos traces, repita os
procedimentos de GDB e ajuste as constantes EXPECTED_* neste script.
MSG
  exit 1
fi
mv "$bin" "$out/mystery-linux-x86_64"
mv "$asm" "$out/mystery.asm"
echo "alvo reconstruido; offsets conferem com os materiais (entry 0x$entry, main 0x$main_start..0x$main_end)"
