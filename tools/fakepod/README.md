# Fake Pod Nano Pro (ESP32-S31) board support

This folder holds what is needed to build the firmware for the **Fake Pod Nano Pro** (ESP32-S31, 460x460 CO5300 AMOLED, CST820 touch, AXP2101, PCF85063, QMI8658, CS43131 headphone DAC, TF slot).
The board layer is switched on with `-DBOARD_FAKEPOD_NANO_PRO=1` (`tools/build_fakepod.sh` does it); without the flag the sources build for the Waveshare board as before.

Files added for the Fake Pod: `rtcbat_fakepod.cpp` (RTC, battery, side keys), `audio_fakepod.cpp`; hunks in `pin_config.h`, `TamaPoke.ino`, `touch.ino` (all small, behind macros); `rtcbat.cpp` and `audio.cpp` are wrapped in `#if !defined(BOARD_FAKEPOD_NANO_PRO)`.

## Build
1. `arduino-cli core install esp32:esp32@4.0.0` (the first release with ESP32-S31 support) and the libraries listed in README.md.
2. `tools/fakepod/patch_arduino_gfx.sh` (adds the S31 to the QSPI driver's chip list in the GFX library).
3. Copy `config_private.example.h` to `config_private.h` and set your time zone (`LOCAL_TZ`, `TZ_STD_OFFSET_H`, `TZ_DST_NA`).
4. `tools/build_fakepod.sh` (add `--upload PORT` to flash). Download mode: hold both side keys while connecting USB.

## Notes
- **The TF card can be FAT32 or exFAT.** arduino-esp32's FatFs has no exFAT, so on this board the card is read through SdFat (MIT) on the ESP-IDF SDMMC host (`sdfat_fs.cpp`, build flag `-DUSE_BLOCK_DEVICE_INTERFACE=1`). The firmware never formats a card.
- Install SdFat 2.3.0 in addition to the libraries in README.md.
- `tools/fakepod/sd_uploader` + `tools/send_sd_fp.py` copy the sprite pack to the card over USB (the game firmware itself does not read USB input).
- The AXP2101 is read-only here: no voltage / rail / charger / power-key-timing register is written. The AMOLED supply is GPIO44, not a PMU rail.
- The RTC chip keeps UTC (shared with other projects on the same board); the game's wall time is derived through the time zone of `config_private.h`.
