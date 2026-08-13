#!/usr/bin/env bash
#
# ide_sim_build.sh — mimic the Arduino IDE build for the Ai-WB2-12F core.
#
# Reads the recipes in platform.txt and executes them the same way the IDE
# would (compile core+variant+sketch, archive core.a, link, objcopy to .bin),
# but with the LINUX toolchain so it can be exercised inside WSL2. This proves
# the platform.txt recipe strings are correct before touching a real IDE.
#
# usage: ide_sim_build.sh <sketch.ino> [build_dir]
set -e

SKETCH="${1:?usage: ide_sim_build.sh <sketch.ino> [build_dir]}"
BUILD="${2:-/tmp/ide_build}"
PLAT=/root/aiwb2-arduino
SDK=/root/wb2-12f-desktop-clock
TOOLBIN="$SDK/toolchain/riscv/Linux/bin"
PROJ="$(basename "${SKETCH%.ino}")"

prop() { grep -m1 "^$1=" "$PLAT/platform.txt" | sed "s/^$1=//"; }

CC="$(prop compiler.c.cmd)"; CXX="$(prop compiler.cpp.cmd)"
AR="$(prop compiler.ar.cmd)"; OBJCOPY="$(prop compiler.objcopy.cmd)"
CFLAGS="$(prop compiler.c.flags)"
CXXFLAGS="$(prop compiler.cpp.flags | sed "s|{toolchain.path}|$SDK/toolchain/riscv/Linux|g")"
DEFS="$(prop compiler.sdk.defines)"
INCS="$(prop compiler.sdk.includes | sed "s|{sdk.path}|$SDK|g")"
ELF_FLAGS="$(prop compiler.c.elf.flags)"
ELF_EXTRA="$(prop compiler.c.elf.extra_flags)"
LIBS="$(prop compiler.sdk.libs | sed "s|{runtime.platform.path}|$PLAT|g; s|{sdk.path}|$SDK|g; s|{toolchain.path}|$SDK/toolchain/riscv/Linux|g")"
OBJCOPY_FLAGS="$(grep -m1 "recipe.objcopy.hex.pattern" "$PLAT/platform.txt" | sed -n 's/.*objcopy.cmd"\([^"]*\)"{build.path}.*/\1/p')"

CORE_INCS="-I$PLAT/cores/arduino -I$PLAT/cores/arduino/include -I$PLAT/variants/wb2-12f -I$BUILD/sketch"

# Library include dirs (mimics the IDE adding each used library's src/).
LIB_INCS=""
# Every src/ directory (recursively) goes on the include path, and every
# source file below src/ gets compiled — matching the Arduino IDE's library
# handling (SdFat, ESP8266WebServer etc. keep .cpp in src/ subdirs).
for libdir in "$PLAT"/libraries/*/src; do
    [ -d "$libdir" ] && LIB_INCS="$LIB_INCS -I$libdir"
    for sub in $(find "$libdir" -type d 2>/dev/null); do
        [ -d "$sub" ] && LIB_INCS="$LIB_INCS -I$sub"
    done
done

rm -rf "$BUILD"; mkdir -p "$BUILD/core" "$BUILD/sketch"

echo "== compile core+variant =="
# Recurse into subdirectories (e.g. cores/arduino/spiffs/) exactly like the
# IDE does, and keep an object name that is unique across directories.
n=0
for src in $(find "$PLAT"/cores/arduino "$PLAT"/variants/wb2-12f \
             -name '*.c' -o -name '*.cpp' | sort); do
    [ -e "$src" ] || continue
    rel="${src#$PLAT/cores/arduino/}"
    obj="$BUILD/core/$(echo "${rel}" | tr '/' '_')".o
    if [[ "$src" == *.cpp ]]; then
        "$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS -c "$src" -o "$obj"
    else
        "$TOOLBIN/$CC" $CFLAGS $DEFS $INCS $CORE_INCS -c "$src" -o "$obj"
    fi
    n=$((n+1))
done
echo "   compiled $n files"

echo "== compile libraries =="
n=0
for src in $(find "$PLAT"/libraries -path '*/src/*' \( -name '*.c' -o -name '*.cpp' \) | sort); do
    [ -e "$src" ] || continue
    libname="$(echo "$src" | sed "s|$PLAT/libraries/||; s|/src/.*||")"
    obj="$BUILD/core/${libname}_$(echo "$src" | sed 's|.*/src/||; s|/|_|g; s|\.\(c\|cpp\)$||').o"
    if [[ "$src" == *.cpp ]]; then
        "$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS -c "$src" -o "$obj"
    else
        "$TOOLBIN/$CC" $CFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS -c "$src" -o "$obj"
    fi
    n=$((n+1))
done
echo "   compiled $n files"

echo "== compile sketch =="
{ echo "#include <Arduino.h>"; cat "$SKETCH"; } > "$BUILD/sketch/$PROJ.ino.cpp"
"$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS -c "$BUILD/sketch/$PROJ.ino.cpp" -o "$BUILD/sketch/$PROJ.ino.o"

echo "== archive core.a =="
"$TOOLBIN/$AR" cru "$BUILD/core/core.a" "$BUILD/core"/*.o

echo "== link =="
"$TOOLBIN/$CC" $ELF_FLAGS $ELF_EXTRA "$BUILD/sketch/$PROJ.ino.o" \
    -Wl,--start-group "$BUILD/core/core.a" $LIBS -Wl,--end-group \
    -o "$BUILD/$PROJ.elf"

echo "== objcopy -> $BUILD/$PROJ.bin =="
"$TOOLBIN/$OBJCOPY" -S -O binary -R .romdata -R .rom -R .bugkiller_command -R .bugkiller \
    "$BUILD/$PROJ.elf" "$BUILD/$PROJ.bin"

echo
"$TOOLBIN/riscv64-unknown-elf-size" "$BUILD/$PROJ.elf"
ls -l "$BUILD/$PROJ.bin"
echo "OK -> $BUILD/$PROJ.bin"
