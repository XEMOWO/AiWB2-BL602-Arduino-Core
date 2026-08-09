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
CXXFLAGS="$(prop compiler.cpp.flags)"
DEFS="$(prop compiler.sdk.defines)"
INCS="$(prop compiler.sdk.includes | sed "s|{sdk.path}|$SDK|g")"
ELF_FLAGS="$(prop compiler.c.elf.flags)"
ELF_EXTRA="$(prop compiler.c.elf.extra_flags)"
LIBS="$(prop compiler.sdk.libs | sed "s|{runtime.platform.path}|$PLAT|g; s|{sdk.path}|$SDK|g")"
OBJCOPY_FLAGS="$(grep -m1 "recipe.objcopy.hex.pattern" "$PLAT/platform.txt" | sed -n 's/.*objcopy.cmd"\([^"]*\)"{build.path}.*/\1/p')"

CORE_INCS="-I$PLAT/cores/arduino -I$PLAT/cores/arduino/include -I$PLAT/variants/wb2-12f -I$BUILD/sketch"

# Library include dirs (mimics the IDE adding each used library's src/).
LIB_INCS=""
for libdir in "$PLAT"/libraries/*/src; do
    [ -d "$libdir" ] && LIB_INCS="$LIB_INCS -I$libdir"
done

rm -rf "$BUILD"; mkdir -p "$BUILD/core" "$BUILD/sketch"

echo "== compile core+variant =="
n=0
for src in "$PLAT"/cores/arduino/*.c "$PLAT"/cores/arduino/*.cpp \
           "$PLAT"/variants/wb2-12f/*.c "$PLAT"/variants/wb2-12f/*.cpp; do
    [ -e "$src" ] || continue
    base="$(basename "$src")"
    obj="$BUILD/core/${base%.*}.o"
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
for src in "$PLAT"/libraries/*/src/*.c "$PLAT"/libraries/*/src/*.cpp; do
    [ -e "$src" ] || continue
    libname="$(basename "$(dirname "$(dirname "$src")")")"
    obj="$BUILD/core/${libname}_$(basename "${src%.*}").o"
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
