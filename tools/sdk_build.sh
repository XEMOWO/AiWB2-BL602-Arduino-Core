#!/usr/bin/env bash
# sdk_build.sh — compile an Arduino sketch for the Ai-WB2-12F (BL602) using
# the Ai-Thinker WB2 SDK (bl_iot_sdk) make system.
#
# This is the build step that platform.txt delegates to. It turns a sketch
# into a flashable .bin the same way the SDK builds any application:
#
#   usage: sdk_build.sh <build_path> <project_name> <sdk_path> <sketch_dir> <core_path>
#
#     build_path  Arduino build directory (where {build.project_name}.bin goes)
#     project_name  e.g. "Blink"
#     sdk_path    root of the Ai-Thinker WB2 SDK (contains applications/, make_scripts_riscv/)
#     sketch_dir  the preprocessed sketch sources ({build.path}/sketch in the IDE)
#     core_path   this package's cores/ directory (has the "arduino" component)
#
# The wrapper creates a temporary SDK application, drops the sketch into it,
# runs `make`, and copies the resulting .bin back to build_path.
set -e

BUILD_PATH="${1:?build_path required}"
PROJECT_NAME="${2:?project_name required}"
SDK_PATH="${3:?sdk_path required}"
SKETCH_DIR="${4:?sketch_dir required}"
CORE_PATH="${5:?core_path required}"

APP_DIR="$SDK_PATH/applications/peripherals/arduino_sketch"
SKETCH_NAME="$PROJECT_NAME"
BIN_DST="$BUILD_PATH/$PROJECT_NAME.bin"

# Use a fixed temp app name so make's dependency cache stays coherent.
TMP_PROJ="arduino_sketch"
BIN_SRC="$APP_DIR/build_out/$TMP_PROJ.bin"

rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/$TMP_PROJ"

# --- 1. template app files (main entry + make config) -----------------------
cat > "$APP_DIR/Makefile" <<EOF
PROJECT_NAME := $TMP_PROJ
PROJECT_PATH := \$(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
EXTRA_COMPONENT_DIRS := $CORE_PATH
-include ./proj_config.mk
ifeq (\$(origin BL60X_SDK_PATH), undefined)
BL60X_SDK_PATH ?= \$(shell pwd)/../../..
endif
COMPONENTS_BLSYS   := bltime blfdt blmtd bloop loopset looprt
COMPONENTS_VFS     := romfs
INCLUDE_COMPONENTS += freertos_riscv_ram
INCLUDE_COMPONENTS += bl602 bl602_std
INCLUDE_COMPONENTS += hosal mbedtls_lts lwip cli vfs yloop utils blog blog_testc newlibc
INCLUDE_COMPONENTS += \$(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += \$(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += \$(COMPONENTS_VFS)
INCLUDE_COMPONENTS += arduino
INCLUDE_COMPONENTS += \$(PROJECT_NAME)
include \$(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
EOF

cat > "$APP_DIR/proj_config.mk" <<'EOF'
####
CONFIG_SYS_VFS_ENABLE:=1
CONFIG_SYS_VFS_UART_ENABLE:=1
# CLI off: aos_cli_init() polls UART0 RX and would steal every byte that
# the sketch's Serial.read() expects (see HardwareSerial.cpp).
CONFIG_SYS_AOS_CLI_ENABLE:=0
CONFIG_SYS_AOS_LOOP_ENABLE:=1
CONFIG_SYS_BLOG_ENABLE:=1
CONFIG_SYS_DMA_ENABLE:=1
CONFIG_SYS_USER_VFS_ROMFS_ENABLE:=0
CONFIG_SYS_APP_TASK_STACK_SIZE:=4096
CONFIG_SYS_APP_TASK_PRIORITY:=15
CONFIG_BL602_USE_ROM_DRIVER:=1
CONFIG_LINK_ROM=1
CONFIG_FREERTOS_TICKLESS_MODE:=0
CONFIG_WIFI:=0
LOG_ENABLED_COMPONENTS:= blog_testc hosal arduino_sketch
EOF

cat > "$APP_DIR/$TMP_PROJ/bouffalo.mk" <<'EOF'
# "main" pseudo-component makefile — compile all sources in this dir.
EOF

cat > "$APP_DIR/$TMP_PROJ/main.c" <<'EOF'
extern void arduino_main(void);
void main(void) { arduino_main(); }
EOF

# --- 2. drop the preprocessed sketch into the app ---------------------------
# The IDE preprocesses <name>.ino -> <name>.ino.cpp. Copy everything so any
# sketch-side headers come along.
cp -f "$SKETCH_DIR"/*.cpp "$APP_DIR/$TMP_PROJ/" 2>/dev/null || true

# --- 3. build ---------------------------------------------------------------
(cd "$APP_DIR" && make -j4 >/dev/null 2>&1) || {
    echo "sdk_build: SDK make failed" >&2
    (cd "$APP_DIR" && make -j4 2>&1 | tail -40) >&2 || true
    exit 1
}

# --- 4. copy the .bin back --------------------------------------------------
[ -f "$BIN_SRC" ] || { echo "sdk_build: $BIN_SRC missing" >&2; exit 1; }
cp -f "$BIN_SRC" "$BIN_DST"
echo "sdk_build: $BIN_DST"
