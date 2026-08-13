#!/usr/bin/env bash
#
# esp_batch_build.sh — batch-compile ESP8266-core examples against the
# Ai-WB2-12F core, mimicking the Arduino IDE build (same platform.txt recipes).
#
# Two phases (so the expensive core+library compile runs once, then many
# sketches are compiled incrementally):
#   esp_batch_build.sh core <build_dir>           # build core.a once
#   esp_batch_build.sh examples <exdir> <build>   # compile every example
#
# Sketch preprocessing mirrors the IDE: a `#include <Arduino.h>` is prepended
# and top-level function prototypes are generated (Arduino's "ctags" step), so
# .ino files that call helpers defined later (NTPClient's sendNTPpacket) build.
#
# WB2_LIBS="WiFi WebServer" limits which platform libraries are compiled.
set -e

MODE="${1:?usage: esp_batch_build.sh (core|examples) ...}"
source "$(dirname "$0")/_build_common.sh"

if [ "$MODE" = "core" ]; then
    BUILD="${2:?usage: esp_batch_build.sh core <build_dir>}"
    rm -rf "$BUILD"; mkdir -p "$BUILD/core" "$BUILD/logs"
    echo "== compile core+variant =="
    n=0
    for src in $(find "$PLAT"/cores/arduino "$PLAT"/variants/wb2-12f \
                 -name '*.c' -o -name '*.cpp' | sort); do
        [ -e "$src" ] || continue
        rel="${src#$PLAT/cores/arduino/}"
        obj="$BUILD/core/$(echo "${rel}" | tr '/' '_')".o
        if [[ "$src" == *.cpp ]]; then
            "$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS -c "$src" -o "$obj" 2>>"$BUILD/logs/core_compile.err" || echo "CORE-FAIL $src" >>"$BUILD/logs/core_compile.err"
        else
            "$TOOLBIN/$CC" $CFLAGS $DEFS $INCS $CORE_INCS -c "$src" -o "$obj" 2>>"$BUILD/logs/core_compile.err" || echo "CORE-FAIL $src" >>"$BUILD/logs/core_compile.err"
        fi
        n=$((n+1))
    done
    echo "   compiled $n core files"
    echo "== compile libraries ($LIB_ACTIVE_JOINED) =="
    n=0
    # Both the modern src/ layout and the legacy root layout (ArduinoOTA).
    # Prune examples/extras/test trees; anything else is a real source file.
    # (A plain -maxdepth 2 cannot be ORed with -path '*/src/*': as a global
    #  depth limit it caps the whole traversal, so src/ files are never seen.)
    for src in $(find "$PLAT"/libraries \
                     \( -path '*/examples/*' -o -path '*/extras/*' -o -path '*/test/*' \) -prune -o \
                     \( -name '*.c' -o -name '*.cpp' \) -print | sort -u); do
        [ -e "$src" ] || continue
        libname="$(echo "$src" | sed "s|$PLAT/libraries/||; s|/src/.*||; s|/.*||")"
        active=0; for a in "${ACTIVE[@]}"; do [ "$a" = "$libname" ] && active=1; done
        [ "$active" = 1 ] || continue
        obj="$BUILD/core/${libname}_$(basename "$src" | sed 's/\.\(c\|cpp\)$//').o"
        if [[ "$src" == *.cpp ]]; then
            "$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS -c "$src" -o "$obj" 2>>"$BUILD/logs/lib_compile.err" || echo "LIB-FAIL $src" >>"$BUILD/logs/lib_compile.err"
        else
            "$TOOLBIN/$CC" $CFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS -c "$src" -o "$obj" 2>>"$BUILD/logs/lib_compile.err" || echo "LIB-FAIL $src" >>"$BUILD/logs/lib_compile.err"
        fi
        n=$((n+1))
    done
    echo "   compiled $n library files"
    "$TOOLBIN/$AR" cru "$BUILD/core/core.a" "$BUILD/core"/*.o
    echo "core.a: $(ls -l "$BUILD/core/core.a" | awk '{print $5}') bytes"
    echo "== core compile errors (if any) =="
    grep -m1 -B1 "error:" "$BUILD/logs/core_compile.err" 2>/dev/null || echo "   (core+libs compiled clean)"
    exit 0
fi

if [ "$MODE" = "examples" ]; then
    EXDIR="${2:?usage: esp_batch_build.sh examples <exdir> <build_dir>}"
    BUILD="${3:?usage: esp_batch_build.sh examples <exdir> <build_dir>}"
    [ -f "$BUILD/core/core.a" ] || { echo "run 'core' phase first"; exit 1; }
    mkdir -p "$BUILD/logs" "$BUILD/sketch"
    SKETCH_INC="-I$BUILD/sketch"

    PASS=0; FAIL=0; FAILED_LIST=()
    # Iterate example FOLDERS, not .ino files: the IDE treats every folder that
    # contains a .ino as ONE sketch, concatenating all other .ino/.cpp tabs in
    # it after the primary .ino (DNSServer/CaptivePortalAdvanced has
    # credentials.ino + tools.ino + handleHttp.ino tabs). The primary is the
    # tab whose basename equals the folder name (else the lone .ino).
    while IFS= read -r dir; do
        mapfile -t inos < <(ls "$dir"/*.ino 2>/dev/null)
        [ ${#inos[@]} -eq 0 ] && continue
        folder="$(basename "$dir")"
        libname="$(echo "$dir" | sed -E "s|$EXDIR/([^/]+)/.*|\1|")"
        # primary + tab ordering (primary first, then other .ino, then .cpp)
        primary=""
        for i in "${inos[@]}"; do [ "$(basename "$i" .ino)" = "$folder" ] && primary="$i"; done
        [ -z "$primary" ] && [ ${#inos[@]} -eq 1 ] && primary="${inos[0]}"
        [ -z "$primary" ] && primary="${inos[0]}"
        tabs=()
        for i in "${inos[@]}"; do [ "$i" != "$primary" ] && tabs+=("$i"); done
        for i in "$dir"/*.cpp; do [ -e "$i" ] && tabs+=("$i"); done

        name="$folder"
        log="$BUILD/logs/${libname}__${name}.log"
        # The IDE hoists every #include out of the sketch, moves top-level
        # typedef/struct definitions ahead of the generated prototypes (so
        # prototypes may reference sketch-defined types like sensorType), then
        # appends the tabs. The example folder itself goes on the include path
        # so quote-includes like "certs.h" resolve. sketch_preprocess.py does
        # the whole job (includes + hoisted defs + prototypes + body).
        python3 "$(dirname "$0")/sketch_preprocess.py" "$primary" "${tabs[@]}" \
            > "$BUILD/sketch/sk.ino.cpp"
        # The example's own library folder goes on the include path (see the
        # matching note in compile_one.sh): it makes angle-bracket relative
        # includes like <../../libraries/ESP8266WiFi/...> resolve back into
        # the examples root, as the IDE does for library examples.
        if "$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS $SKETCH_INC -I"$dir" \
            -I"$EXDIR/$libname" \
            -c "$BUILD/sketch/sk.ino.cpp" -o "$BUILD/sketch/sk.o" >"$log" 2>&1; then
            if "$TOOLBIN/$CC" $ELF_FLAGS $ELF_EXTRA "$BUILD/sketch/sk.o" \
                -Wl,--start-group "$BUILD/core/core.a" $LIBS -Wl,--end-group \
                -o "$BUILD/sketch/sk.elf" >>"$log" 2>&1; then
                PASS=$((PASS+1)); echo "PASS  $libname/$name"
            else
                FAIL=$((FAIL+1)); echo "FAIL(L) $libname/$name"; FAILED_LIST+=("$libname__$name")
            fi
        else
            FAIL=$((FAIL+1)); echo "FAIL(C) $libname/$name"; FAILED_LIST+=("$libname__$name")
        fi
    done < <(find "$EXDIR" -type d | sort)

    echo; echo "================================"
    echo "TOTAL: $((PASS+FAIL))  PASS: $PASS  FAIL: $FAIL"
    if [ "$FAIL" -gt 0 ]; then
        echo "--- first error of each failure ---"
        for f in "${FAILED_LIST[@]}"; do
            log="$BUILD/logs/${f}.log"
            echo "### $f"
            grep -m1 -E 'error:|fatal error:|undefined reference|collect2:' "$log" 2>/dev/null || echo "   (no error line)"
        done
    fi
    exit 0
fi

echo "unknown mode $MODE"; exit 1
