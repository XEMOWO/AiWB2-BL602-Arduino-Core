#!/usr/bin/env bash
#
# compile_one.sh — compile ONE ESP8266 example against the WB2 core, same
# recipe as esp_batch_build.sh but fast for iterating on a single sketch.
#
#   compile_one.sh <exdir> <build> <libname/ExampleName> [--no-link]
#
#   exdir    : the examples root (mirror of .../esp8266/3.0.2/libraries)
#   build    : build dir (must contain core/core.a from the 'core' phase)
#   libname/ExampleName : e.g. ESP8266WebServer/HelloServer
#
# Prints PASS/FAIL(C)/FAIL(L) and leaves the full log in <build>/logs/one.log.
# With --no-link, stops after -c (compile-only) — use while iterating on
# missing-API errors, then drop the flag to catch link problems too.
set -e

PLAT=/root/aiwb2-arduino
source "$(dirname "$0")/_build_common.sh"

EXDIR="${1:?usage: compile_one.sh <exdir> <build> <lib/Example> [--no-link]}"
BUILD="${2:?usage}"
TARGET="${3:?usage}"
NOLINK=0; [ "${4:-}" = "--no-link" ] && NOLINK=1

[ -f "$BUILD/core/core.a" ] || { echo "run 'core' phase first"; exit 1; }
mkdir -p "$BUILD/logs" "$BUILD/sketch"
SKETCH_INC="-I$BUILD/sketch"

libname="${TARGET%%/*}"
name="${TARGET##*/}"
if [ -d "$EXDIR/$TARGET" ]; then
    dir="$EXDIR/$TARGET"
elif [ -d "$EXDIR/$libname/examples/$name" ]; then
    dir="$EXDIR/$libname/examples/$name"
else
    echo "no such example dir: $TARGET"; exit 1
fi

mapfile -t inos < <(ls "$dir"/*.ino 2>/dev/null)
folder="$name"
primary=""
for i in "${inos[@]}"; do [ "$(basename "$i" .ino)" = "$folder" ] && primary="$i"; done
[ -z "$primary" ] && primary="${inos[0]}"
tabs=()
for i in "${inos[@]}"; do [ "$i" != "$primary" ] && tabs+=("$i"); done
for i in "$dir"/*.cpp; do [ -e "$i" ] && tabs+=("$i"); done

log="$BUILD/logs/one.log"
python3 "$(dirname "$0")/sketch_preprocess.py" "$primary" "${tabs[@]}" \
    > "$BUILD/sketch/sk.ino.cpp"

# The example's own library folder goes on the include path too: sketches use
# angle-bracket relative includes like <../../libraries/ESP8266WiFi/...>
# (lwIP_Ethernet/EthSSLValidation pulls in the BearSSL_Validation example).
# From <EXDIR>/<libname> that path resolves back to <EXDIR>/libraries/..., as
# the Arduino IDE does when it attaches a library while building its example.
if "$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS $SKETCH_INC -I"$dir" \
    -I"$EXDIR/$libname" \
    -c "$BUILD/sketch/sk.ino.cpp" -o "$BUILD/sketch/sk.o" >"$log" 2>&1; then
    if [ "$NOLINK" = 1 ]; then
        echo "PASS(C) $TARGET (compile only)"; exit 0
    fi
    if "$TOOLBIN/$CC" $ELF_FLAGS $ELF_EXTRA "$BUILD/sketch/sk.o" \
        -Wl,--start-group "$BUILD/core/core.a" $LIBS -Wl,--end-group \
        -o "$BUILD/sketch/sk.elf" >>"$log" 2>&1; then
        echo "PASS  $TARGET"
    else
        echo "FAIL(L) $TARGET — log: $log"
    fi
else
    echo "FAIL(C) $TARGET — log: $log"
fi
