#!/usr/bin/env bash
# flash.sh — flash a .bin to the Ai-WB2-12F via the SDK's bflb_iot_tool.
#
#   usage: flash.sh <firmware.bin> [port]
#
# BL602 has a ROM UART bootloader; the tool takes the chip into boot mode
# automatically. Port defaults to /dev/ttyUSB0.
set -e

SDK=/root/wb2-12f-desktop-clock
BIN="${1:?usage: flash.sh <firmware.bin> [port]}"
PORT="${2:-/dev/ttyUSB0}"
FT="$SDK/tools/flash_tool"

[ -f "$BIN" ] || { echo "firmware not found: $BIN" >&2; exit 1; }
[ -e "$PORT" ] || { echo "port not present: $PORT  (board not connected / not attached to WSL2?)" >&2; exit 1; }

LOG=$(mktemp)
"$FT/bflb_iot_tool-ubuntu" --chipname=BL602 --port="$PORT" --baudrate=921600 \
  --pt="$FT/chips/bl602/partition/partition_cfg_2M.toml" \
  --dts="$FT/chips/bl602/device_tree/04-bl_factory_params_IoTKitA_40M-20220625.dts" \
  --boot2="$FT/chips/bl602/builtin_imgs/boot2_isp_bl602_v6.5.1/boot2_iap_release.bin" \
  --firmware="$BIN" 2>&1 | tee "$LOG"

if grep -qE "Burn return with retry fail|shake hand fail|ErrorCode" "$LOG"; then
    echo "!!! FLASH FAILED — chip did not enter bootloader mode." >&2
    echo "!!! Hold BOOT, tap RST, release BOOT, then retry." >&2
    exit 1
fi

echo "Flash OK. Watch the console with:"
echo "  python3 /root/aiwb2-arduino/tools/serial_monitor.py --port $PORT --baud 2000000"
