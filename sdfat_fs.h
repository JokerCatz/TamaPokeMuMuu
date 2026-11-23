// Fake Pod Nano Pro: an exFAT-capable replacement for arduino-esp32's SD_MMC.
// Why: arduino-esp32's FatFs is precompiled with FF_FS_EXFAT 0 (and Espressif deliberately gives no switch for it), so a card as sold at 64 GB and up does not mount. Here the file
// system is SdFat's (MIT, Bill Greiman; reads FAT16/32 and exFAT) sitting on ESP-IDF's SDMMC host driver as a generic block device: 4-bit, no SPI bus, no FatFs. Needs the build flag
// -DUSE_BLOCK_DEVICE_INTERFACE=1 (tools/build_fakepod.sh sets it). The class keeps the SD_MMC calls the game makes (open, exists, remove, rename, mkdir, begin, setPins, cardSize,
// totalBytes, usedBytes) and the fs::File API, so the game's code is unchanged. It NEVER formats a card.
// A card that was ever put in SPI mode stays there until it loses power (take it out and put it back): SDMMC init then times out.
#pragma once
#if defined(BOARD_FAKEPOD_NANO_PRO)
#include <FS.h>
#include <SdFat.h>

class SdFatFS : public fs::FS {
public:
  SdFatFS();
  bool setPins(int clk, int cmd, int d0);                     // the other data lines come from pin_config.h (SDMMC_D1 / D2 / D3)
  bool begin(const char *mountpoint = "/sdcard", bool mode1bit = false, bool formatIfMountFailed = false);  // the arguments are ignored: 4-bit, and never a format
  uint64_t cardSize();
  uint64_t totalBytes();
  uint64_t usedBytes();                                       // scans the allocation table: slow on a big card, console use only
  FsVolume *volume() { return &vol_; }
  bool read(uint32_t sector, uint8_t *dst, size_t n);        // used by the block device
  bool write(uint32_t sector, const uint8_t *src, size_t n);
  uint32_t sectors();
private:
  FsVolume vol_;
  int clk_ = -1, cmd_ = -1, d0_ = -1;
  bool cardOk_ = false;
  friend class SdFatFSImpl;
  friend class SdmmcDevice;
};
extern SdFatFS SD_MMC;
#endif
