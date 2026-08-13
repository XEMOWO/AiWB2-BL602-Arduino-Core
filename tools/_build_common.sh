#!/usr/bin/env bash
#
# _build_common.sh — shared recipe/flags for the WB2 core builds. Sourced by
# esp_batch_build.sh and compile_one.sh so every path resolves identically.
# Expects `set -e` already in the caller; defines PLAT..LIB_ACTIVE_JOINED.

PLAT=/root/aiwb2-arduino
SDK=/root/wb2-12f-desktop-clock
TOOLBIN="$SDK/toolchain/riscv/Linux/bin"

prop() { grep -m1 "^$1=" "$PLAT/platform.txt" | sed "s/^$1=//"; }
CC="$(prop compiler.c.cmd)"; CXX="$(prop compiler.cpp.cmd)"
AR="$(prop compiler.ar.cmd)"
FIX32="$(prop wb2.int32.fix | sed "s|{runtime.platform.path}|$PLAT|g")"
CFLAGS="$(prop compiler.c.flags | sed "s|{wb2.int32.fix}|$FIX32|g; s|{toolchain.path}|$SDK/toolchain/riscv/Linux|g")"
CXXFLAGS="$(prop compiler.cpp.flags | sed "s|{toolchain.path}|$SDK/toolchain/riscv/Linux|g; s|{wb2.int32.fix}|$FIX32|g")"
DEFS="$(prop compiler.sdk.defines)"
INCS="$(prop compiler.sdk.includes | sed "s|{sdk.path}|$SDK|g")"
ELF_FLAGS="$(prop compiler.c.elf.flags)"
ELF_EXTRA="$(prop compiler.c.elf.extra_flags)"
LIBS="$(prop compiler.sdk.libs | sed "s|{runtime.platform.path}|$PLAT|g; s|{sdk.path}|$SDK|g; s|{toolchain.path}|$SDK/toolchain/riscv/Linux|g")"

CORE_INCS="-I$PLAT/cores/arduino -I$PLAT/cores/arduino/include -I$PLAT/variants/wb2-12f"

# ---- library include dirs (all active libs' src/ trees) -------------------
# Libraries may use the modern src/ layout or the legacy root layout
# (ArduinoOTA, ESP8266SSDP & friends put ArduinoOTA.h in the lib root).
ALL_LIBS=($(ls "$PLAT"/libraries))
if [ -n "$WB2_LIBS" ]; then ACTIVE=($WB2_LIBS); else ACTIVE=("${ALL_LIBS[@]}"); fi
LIB_INCS=""
for lib in "${ACTIVE[@]}"; do
    libdir="$PLAT/libraries/$lib/src"
    [ -d "$libdir" ] || libdir="$PLAT/libraries/$lib"
    [ -d "$libdir" ] || continue
    LIB_INCS="$LIB_INCS -I$libdir"
    for sub in $(find "$libdir" -type d 2>/dev/null); do
        [ -d "$sub" ] && LIB_INCS="$LIB_INCS -I$sub"
    done
done
LIB_ACTIVE_JOINED="$(IFS=,; echo "${ACTIVE[*]}")"

# Arduino's ctags step: emit a prototype for every top-level function
# definition so a helper used before its definition (NTPClient etc.) builds.
proto() {
    python3 - "$1" <<'PY'
import re, sys
src = open(sys.argv[1], encoding='utf-8', errors='replace').read()
# strip // and block comments (naive but fine for prototype detection)
src = re.sub(r'//[^\n]*', '', src)
src = re.sub(r'/\*.*?\*/', '', src, flags=re.S)
# remove preprocessor lines so they don't confuse brace counting
src = '\n'.join(l for l in src.split('\n') if not l.lstrip().startswith('#'))
depth = 0
funcs = []
keywords = ('if','for','while','switch','catch','else','do','return','new','delete')
text = ''
# walk char by char tracking brace depth; collect text at depth 0
for ch in src:
    if ch == '{':
        if depth == 0:
            # candidate function body starts here
            sig = text.rstrip().rstrip(';').strip()
            # A top-level global variable statement ends in ';', but a function
            # signature cannot contain a top-level ';'. Drop everything through
            # the last ';' so that "int* nullPointer = NULL;\n" before
            # "void panic(...)" is NOT copied into the emitted prototype (which
            # would re-declare the variable). re.S (DOTALL) lets '.' span the
            # newlines inside a multi-line signature.
            semi = sig.rfind(';')
            if semi >= 0:
                sig = sig[semi + 1:].lstrip()
            m = re.match(r'(template\s*<[^>]*>)?\s*(.*?\b)([A-Za-z_]\w*)\s*\(([^;]*)\)\s*$', sig, flags=re.S)
            if m and m.group(3) not in keywords and '(' in sig:
                tpl, rest, name, params = m.groups()
                # skip class/struct/namespace decls and assignments
                if not re.match(r'\s*(class|struct|enum|namespace|using|typedef)\b', rest) and not re.search(r'=\s*$', rest):
                    funcs.append((tpl or '', rest, name, params))
        depth += 1
    elif ch == '}':
        depth = max(0, depth - 1)
        if depth == 0:
            text = ''
    if depth == 0 and ch != '{' and ch != '}':
        text += ch
    elif depth > 0:
        text = ''
for tpl, rest, name, params in funcs:
    if name in ('setup', 'loop', 'main'):
        continue
    # normalize whitespace and re-emit as a forward declaration
    print(f"{tpl} {rest}{name}({params});")
PY
}

# xtensa_filter <file> — drop Xtensa-only inline-asm from example sources.
# Sketches like HwdtStackDump/IramReserve embed Xtensa instructions
# (`mov.n a2, %0` / `break 1, 15`) to crash the CPU on demand. There is no
# RISC-V equivalent, so a statement whose string mentions an Xtensa mnemonic is
# replaced with a no-op comment (the hot-key simply does nothing). Portable asm
# (`asm volatile("" ::: "memory")`, RISC-V asm, etc.) is preserved.
xtensa_filter() {
    python3 - "$1" <<'PY'
import re, sys
src = open(sys.argv[1], encoding='utf-8', errors='replace').read()
xt = re.compile(r'\b(mov\.n|mov\.a|movi\.n|l32i(?:\.n)?|s32i(?:\.n)?|'
                r'wsr\.|rsr\.|break|extui|retw|call0|mux\b|bnone|beqz|bnez|'
                r'bbsi|bbci|ssai|sext|slli|srli|sub\.)\b')
out = []
i, n = 0, len(src)
pat = re.compile(r'\b(?:__asm__|asm)\s+(?:volatile|__volatile__)?\s*\(')
while i < n:
    m = pat.search(src, i)
    if not m:
        out.append(src[i:]); break
    out.append(src[i:m.start()])
    depth, j = 0, m.end() - 1
    while j < n:
        if src[j] == '(': depth += 1
        elif src[j] == ')':
            depth -= 1
            if depth == 0: break
        j += 1
    stmt = src[m.start():j + 1]
    out.append('/* Xtensa asm neutralized on WB2 (RISC-V) */' if xt.search(stmt) else stmt)
    i = j + 1
sys.stdout.write(''.join(out))
PY
}
