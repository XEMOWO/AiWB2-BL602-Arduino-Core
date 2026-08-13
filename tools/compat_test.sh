#!/usr/bin/env bash
#
# compat_test.sh — verify ESP8266 third-party library compatibility.
#
# Compiles each library's sources against the Ai-WB2-12F core headers exactly
# the way the Arduino IDE would (same flags, same include paths), WITHOUT
# linking — it is a pure "does this library compile on WB2?" check. This is the
# acceptance harness for the ESP8266-compat work: every phase's output is
# validated by batch-compiling real ESP8266 libraries.
#
# usage: compat_test.sh <libdir> [<libdir> ...]
set -u

PLAT=/root/aiwb2-arduino
SDK=/root/wb2-12f-desktop-clock
TOOLBIN="$SDK/toolchain/riscv/Linux/bin"

prop() { grep -m1 "^$1=" "$PLAT/platform.txt" | sed "s/^$1=//"; }
CXX="$(prop compiler.cpp.cmd)"; CC="$(prop compiler.c.cmd)"
CXXFLAGS="$(prop compiler.cpp.flags | sed "s|{toolchain.path}|$SDK/toolchain/riscv/Linux|g")"; CFLAGS="$(prop compiler.c.flags)"
DEFS="$(prop compiler.sdk.defines)"
INCS="$(prop compiler.sdk.includes | sed "s|{sdk.path}|$SDK|g")"
CORE_INCS="-I$PLAT/cores/arduino -I$PLAT/variants/wb2-12f"

# Every installed core library's src/ is on the include path (mimics the IDE).
LIB_INCS=""
for libdir in "$PLAT"/libraries/*/src; do
    [ -d "$libdir" ] && LIB_INCS="$LIB_INCS -I$libdir"
done

# Optional extra include dirs for external dependencies (Adafruit_Sensor etc.),
# space-separated:  ESP_EXTRA_INCS="/path/a /path/b" ./compat_test.sh ...
for d in ${ESP_EXTRA_INCS:-}; do
    [ -d "$d" ] && LIB_INCS="$LIB_INCS -I$d"
done

PASS=0; FAIL=0; SKIP=0
for LIB in "$@"; do
    [ -d "$LIB" ] || { echo "SKIP  $LIB (not a directory)"; SKIP=$((SKIP+1)); continue; }

    # collect sources: prefer <lib>/src/, else top level. The real Arduino
    # build recurses into src/ subdirectories (SdFat, Adafruit libs, ...), so
    # collect recursively and put every source directory on the include path.
    srcs=()
    stub=0
    if [ -d "$LIB/src" ]; then
        while IFS= read -r -d '' f; do srcs+=("$f"); done < <(find "$LIB/src" \( -name '*.cpp' -o -name '*.c' \) -print0 | sort -z)
        LIBINC="-I$LIB/src"
        while IFS= read -r -d '' d; do LIBINC="$LIBINC -I$d"; done < <(find "$LIB/src" -type d -print0)
    else
        while IFS= read -r -d '' f; do srcs+=("$f"); done < <(find "$LIB" -maxdepth 1 \( -name '*.cpp' -o -name '*.c' \) -print0 | sort -z)
        LIBINC="-I$LIB"
    fi
    # Header-only libraries (e.g. ArduinoJson): no .cpp to compile, but the
    # header must still parse. Compile a generated stub that includes the
    # library's entry header — the one the user actually includes, matched by
    # name (ESP8266WebServer.h, ArduinoJson.h, ...). Includes only that one:
    # including every top-level header can double-include headers that pull in
    # a common implementation header (e.g. ESP8266WebServer.h pulls in
    # ESP8266WebServer-impl.h), producing false redefinition errors the real
    # IDE never sees.
    if [ ${#srcs[@]} -eq 0 ]; then
        HDRDIR="$LIB/src"; [ -d "$HDRDIR" ] || HDRDIR="$LIB"
        entry="$(basename "$LIB")"".h"
        if [ -e "$HDRDIR/$entry" ]; then
            stub="/tmp/compat_stub_$$.cpp"
            printf '#include "%s"\n' "$entry" > "$stub"
            srcs+=("$stub")
        else
            hdrs=("$HDRDIR"/*.h)
            if [ -e "${hdrs[0]}" ]; then
                stub="/tmp/compat_stub_$$.cpp"
                printf '#include "%s"\n' "$(basename "${hdrs[0]}")" > "$stub"
                srcs+=("$stub")
            else
                echo "SKIP  $LIB (no .cpp/.c/.h found)"; SKIP=$((SKIP+1)); continue
            fi
        fi
    fi

    libfail=0
    for src in "${srcs[@]}"; do
        base="$(basename "$src")"
        if [[ "$src" == *.cpp ]]; then
            out=$("$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS $LIBINC \
                -c "$src" -o "/tmp/compat_obj.o" 2>&1)
        else
            out=$("$TOOLBIN/$CC" $CFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS $LIBINC \
                -c "$src" -o "/tmp/compat_obj.o" 2>&1)
        fi
        if [ $? -ne 0 ]; then
            libfail=1
            err=$(echo "$out" | grep -m2 'error:' | sed -E 's/.*error: /error: /' | tr '\n' ' ')
            echo "  ✗ $base: ${err:0:200}"
        fi
    done

    if [ $libfail -eq 0 ]; then
        echo "PASS  $LIB"
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
    fi
done
rm -f /tmp/compat_obj.o
echo "----------------------------------------------------------------"
echo "PASS=$PASS  FAIL=$FAIL  SKIP=$SKIP"
