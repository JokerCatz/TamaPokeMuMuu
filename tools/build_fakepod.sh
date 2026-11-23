#!/usr/bin/env bash
# Builds the firmware for the Fake Pod Nano Pro (ESP32-S31) with arduino-cli + arduino-esp32 4.0.0. The sketch is copied to a scratch folder first (arduino-cli wants the folder
# named like the main .ino, and the build output stays out of this possibly-synced folder).  tools/build_fakepod.sh [--upload PORT]
# Needs: arduino-cli, the esp32:esp32@4.0.0 core, the libraries pinned in README.md (GFX Library for Arduino with ESP32S31 added to the Arduino_ESP32QSPI chip list, see
# tools/fakepod/README.md) and SdFat 2.3.0.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; C="${TAMAPOKE_BUILD_DIR:-$HOME/.cache/tamapoke-mumuu}"
export ARDUINO_CONFIG_FILE="${ARDUINO_CONFIG_FILE:-$HOME/.arduino15/arduino-cli.yaml}"
FQBN="esp32:esp32:esp32s31:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB"
rm -rf "$C/TamaPoke"; mkdir -p "$C/TamaPoke" "$C/build"
(cd "$R" && cp *.ino *.cpp *.h "$C/TamaPoke/"; ls *.c >/dev/null 2>&1 && cp *.c "$C/TamaPoke/"; [ -d bsp ] && cp -r bsp "$C/TamaPoke/"; [ -d pc ] && cp -r pc "$C/TamaPoke/"; true)
FLAGS="-DBOARD_FAKEPOD_NANO_PRO=1 -DUSE_BLOCK_DEVICE_INTERFACE=1"   # the second: SdFat on a generic block device (sdfat_fs.cpp)
cd "$C"
arduino-cli compile --fqbn "$FQBN" --build-path "$C/build" --warnings none \
  --build-property "compiler.cpp.extra_flags=$FLAGS" --build-property "compiler.c.extra_flags=$FLAGS" TamaPoke
if [ "${1:-}" = "--upload" ]; then
  arduino-cli upload --fqbn "$FQBN" -p "$2" --input-dir "$C/build" TamaPoke
fi
