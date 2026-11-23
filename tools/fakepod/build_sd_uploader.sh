#!/usr/bin/env bash
# Builds (and with --upload PORT flashes) the SD uploader firmware, see tools/fakepod/sd_uploader/sd_uploader.ino.
set -euo pipefail
R="$(cd "$(dirname "$0")/../.." && pwd)"; C="${TAMAPOKE_BUILD_DIR:-$HOME/.cache/tamapoke-mumuu}/SdUploader"
export ARDUINO_CONFIG_FILE="${ARDUINO_CONFIG_FILE:-$HOME/.arduino15/arduino-cli.yaml}"
FQBN="esp32:esp32:esp32s31:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB"
rm -rf "$C"; mkdir -p "$C/SdUploader" "$C/build"
cp "$R/tools/fakepod/sd_uploader/sd_uploader.ino" "$C/SdUploader/SdUploader.ino"; cp "$R/pin_config.h" "$R/sdfat_fs.h" "$R/sdfat_fs.cpp" "$C/SdUploader/"
FLAGS="-DBOARD_FAKEPOD_NANO_PRO=1 -DUSE_BLOCK_DEVICE_INTERFACE=1"
cd "$C"
arduino-cli compile --fqbn "$FQBN" --build-path "$C/build" --warnings none --build-property "compiler.cpp.extra_flags=$FLAGS" --build-property "compiler.c.extra_flags=$FLAGS" SdUploader
[ "${1:-}" = "--upload" ] && arduino-cli upload --fqbn "$FQBN" -p "$2" --input-dir "$C/build" SdUploader
true
