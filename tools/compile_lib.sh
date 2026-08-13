#!/usr/bin/env bash
#
# compile_lib.sh <libname> — compile (not link/archive) every .c/.cpp in one
# platform library against the core, with the same flags as the 'core' phase.
# Fast iteration for porting a library: fix errors until "LIB-CLEAN".
#
#   WB2_LIBS="DNSServer" bash tools/compile_lib.sh DNSServer   # isolate include set
set -e
PLAT=/root/aiwb2-arduino
source "$(dirname "$0")/_build_common.sh"

LIB="${1:?usage: compile_lib.sh <libname>}"
[ -d "$PLAT/libraries/$LIB" ] || { echo "no such library: $LIB"; exit 1; }
out=/tmp/libcheck; rm -rf "$out"; mkdir -p "$out"

rc=0
for src in $(find "$PLAT/libraries/$LIB" -type f \( -name '*.c' -o -name '*.cpp' \) | sort); do
    if [[ "$src" == *.cpp ]]; then
        "$TOOLBIN/$CXX" $CXXFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS -c "$src" -o "$out/$(basename "${src%.cpp}").o" || rc=1
    else
        "$TOOLBIN/$CC" $CFLAGS $DEFS $INCS $CORE_INCS $LIB_INCS -c "$src" -o "$out/$(basename "${src%.c}").o" || rc=1
    fi
done

[ "$rc" = 0 ] && echo "LIB-CLEAN $LIB" || echo "LIB-FAIL $LIB (see errors above)"
exit "$rc"
