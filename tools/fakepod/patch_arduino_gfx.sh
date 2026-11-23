#!/usr/bin/env bash
# Adds ESP32-S31 to the chip allow-list of the QSPI bus driver in the "GFX Library for Arduino" (1.6.9): its Arduino_ESP32QSPI.{h,cpp} and Arduino_GFX_Library.h only compile for
# the chips they name, and the S31 is not one of them yet (arduino-esp32 4.0.0 supports the S31). Idempotent. Only the QSPI driver is opened up: the plain-SPI driver pokes
# S3-specific registers and must stay excluded.   tools/fakepod/patch_arduino_gfx.sh [library dir]
set -euo pipefail
L="${1:-$HOME/Arduino/libraries/GFX_Library_for_Arduino}/src"
for f in databus/Arduino_ESP32QSPI.h databus/Arduino_ESP32QSPI.cpp Arduino_GFX_Library.h; do
  [ -f "$L/$f" ] || { echo "missing $L/$f" >&2; exit 1; }
  grep -q CONFIG_IDF_TARGET_ESP32S31 "$L/$f" && { echo "already patched: $f"; continue; }
  sed -i 's/CONFIG_IDF_TARGET_ESP32C5)/CONFIG_IDF_TARGET_ESP32C5 || CONFIG_IDF_TARGET_ESP32S31)/g; s/CONFIG_IDF_TARGET_ESP32C5 *$/CONFIG_IDF_TARGET_ESP32C5 || CONFIG_IDF_TARGET_ESP32S31/' "$L/$f"
  echo "patched: $f"
done
