// SD uploader firmware for the Fake Pod Nano Pro: a tiny sketch that mounts the TF card (exFAT too, SdFat on the SDMMC host, see sdfat_fs.cpp) and speaks the game's USB upload
// protocol, so the sprite pack can be copied to the card without taking the card out:
//   LS\n                       -> "name size" per file in /mons, then DONE
//   PUT <path> <bytes>\n + raw -> OK, "#" after each 2 KB block, DONE   (only under /mons/, never anywhere else; an existing file is replaced)
// Build and flash it with tools/fakepod/build_sd_uploader.sh, send with tools/send_sd_fp.py, then flash the game again. It never formats a card.
#include <Arduino.h>
#include "pin_config.h"
#include "sdfat_fs.h"

static bool sdReady = false;

static void cmdLS() {
  if (!sdReady) { Serial.println("ERR"); return; }
  File dir = SD_MMC.open("/mons");
  if (dir) {
    File e;
    while ((e = dir.openNextFile())) {
      Serial.printf("%s %u\n", e.name(), (uint32_t)e.size());
      e.close();
    }
    dir.close();
  }
  Serial.println("DONE");
}

static void cmdPUT(const String &line) {
  int sp = line.lastIndexOf(' ');
  String path = line.substring(4, sp);
  uint32_t size = line.substring(sp + 1).toInt();
  if (!sdReady || size == 0 || size > 4 * 1024 * 1024) { Serial.println("ERR"); return; }
  if (!path.startsWith("/")) path = "/" + path;
  if (!path.startsWith("/mons/") || path.indexOf("..") >= 0) { Serial.println("ERR"); return; }  // writes are confined to /mons/
  if (SD_MMC.exists(path)) SD_MMC.remove(path);
  File f = SD_MMC.open(path, FILE_WRITE);
  if (!f) { Serial.println("ERR"); return; }
  Serial.println("OK");
  static uint8_t buf[2048];
  uint32_t remaining = size;
  Serial.setTimeout(5000);
  while (remaining > 0) {
    size_t want = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    size_t n = Serial.readBytes(buf, want);
    if (n == 0) break;
    if (f.write(buf, n) != n) break;
    remaining -= n;
    Serial.println("#");
  }
  f.close();
  Serial.setTimeout(1000);
  Serial.println(remaining == 0 ? "DONE" : "ERR");
}

void setup() {
  Serial.setRxBufferSize(8192);
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(500);
  Serial.println("sd_uploader: ready");
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  sdReady = SD_MMC.begin("/sdcard", true, false);
  Serial.printf("SD %s\n", sdReady ? "mounted" : "NOT mounted");
  if (sdReady) { Serial.printf("SD montada: %llu MB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL)); SD_MMC.mkdir("/mons"); }
}

void loop() {
  if (!Serial.available()) { delay(2); return; }
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line == "LS") cmdLS();
  else if (line.startsWith("PUT ")) cmdPUT(line);
}
