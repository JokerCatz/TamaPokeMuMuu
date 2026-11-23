// See sdfat_fs.h.
#if defined(BOARD_FAKEPOD_NANO_PRO)
#include "sdfat_fs.h"
#include "pin_config.h"
#include <FSImpl.h>   // fs::FileImpl / fs::FSImpl (the base classes FS.h only forward-declares)
#include <memory>
#include <utility>
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

#if !USE_BLOCK_DEVICE_INTERFACE
#error "sdfat_fs.cpp needs -DUSE_BLOCK_DEVICE_INTERFACE=1 (see tools/build_fakepod.sh)"
#endif

static sdmmc_card_t g_card;
static SemaphoreHandle_t g_lock;   // SdFat's volume (one sector cache) is not thread safe: every call below holds this recursive mutex
struct Guard { Guard() { if (g_lock) xSemaphoreTakeRecursive(g_lock, portMAX_DELAY); } ~Guard() { if (g_lock) xSemaphoreGiveRecursive(g_lock); } };

// the card as a generic block device for SdFat
class SdmmcDevice : public FsBlockDeviceInterface {
public:
  bool isBusy() override { return false; }
  bool readSector(uint32_t s, uint8_t *d) override { return readSectors(s, d, 1); }
  bool readSectors(uint32_t s, uint8_t *d, size_t n) override { return sdmmc_read_sectors(&g_card, d, s, n) == ESP_OK; }
  uint32_t sectorCount() override { return g_card.csd.capacity; }
  bool syncDevice() override { return true; }
  bool writeSector(uint32_t s, const uint8_t *d) override { return writeSectors(s, d, 1); }
  bool writeSectors(uint32_t s, const uint8_t *d, size_t n) override { return sdmmc_write_sectors(&g_card, d, s, n) == ESP_OK; }
};
static SdmmcDevice g_dev;

namespace {
String baseName(const String &p) { int i = p.lastIndexOf('/'); return i >= 0 ? p.substring(i + 1) : p; }

class SdFatFileImpl : public fs::FileImpl {
public:
  SdFatFileImpl(FsFile &&f, const String &path, const String &name) : f_(std::move(f)), path_(path), name_(name) {}
  size_t write(const uint8_t *buf, size_t size) override { Guard g; return f_.write(buf, size); }
  size_t read(uint8_t *buf, size_t size) override { Guard g; int n = f_.read(buf, size); return n < 0 ? 0 : (size_t)n; }
  void flush() override { Guard g; f_.sync(); }
  bool seek(uint32_t pos, fs::SeekMode mode) override {
    Guard g;
    switch (mode) {
      case fs::SeekSet: return f_.seekSet(pos);
      case fs::SeekCur: return f_.seekCur(pos);
      default: return f_.seekEnd(pos);
    }
  }
  size_t position() const override { Guard g; return (size_t)const_cast<FsFile &>(f_).curPosition(); }
  size_t size() const override { Guard g; return (size_t)const_cast<FsFile &>(f_).fileSize(); }
  bool setBufferSize(size_t) override { return false; }
  void close() override { Guard g; f_.close(); }
  time_t getLastWrite() override { return 0; }
  const char *path() const override { return path_.c_str(); }
  const char *name() const override { return name_.c_str(); }
  boolean isDirectory(void) override { return f_.isDir(); }
  fs::FileImplPtr openNextFile(const char *) override {
    Guard g;
    FsFile n;
    if (!f_.isDir() || !n.openNext(&f_, O_RDONLY)) return fs::FileImplPtr();
    char nm[256];
    n.getName(nm, sizeof nm);
    String full = path_;
    if (!full.endsWith("/")) full += "/";
    full += nm;
    return std::make_shared<SdFatFileImpl>(std::move(n), full, String(nm));
  }
  boolean seekDir(long) override { return false; }
  String getNextFileName(void) override { bool d; return getNextFileName(&d); }
  String getNextFileName(bool *isDir) override {
    Guard g;
    FsFile n;
    if (!f_.isDir() || !n.openNext(&f_, O_RDONLY)) return String();
    char nm[256];
    n.getName(nm, sizeof nm);
    if (isDir) *isDir = n.isDir();
    n.close();
    String full = path_;
    if (!full.endsWith("/")) full += "/";
    return full + nm;
  }
  void rewindDirectory(void) override { Guard g; f_.rewindDirectory(); }
  operator bool() override { return f_.isOpen(); }
private:
  FsFile f_;
  String path_, name_;
};
}  // namespace

class SdFatFSImpl : public fs::FSImpl {
public:
  explicit SdFatFSImpl(SdFatFS *o) : o_(o) {}
  fs::FileImplPtr open(const char *path, const char *mode, const bool create) override {
    Guard g;
    oflag_t flags = O_RDONLY;
    if (mode && mode[0] == 'w') flags = O_WRONLY | O_CREAT | O_TRUNC;
    else if (mode && mode[0] == 'a') flags = O_WRONLY | O_CREAT | O_APPEND;
    if (mode && strchr(mode, '+')) flags = (flags & ~O_WRONLY) | O_RDWR;
    if (create) flags |= O_CREAT;
    FsFile f = o_->vol_.open(path, flags);
    if (!f) return fs::FileImplPtr();
    return std::make_shared<SdFatFileImpl>(std::move(f), String(path), baseName(String(path)));
  }
  bool exists(const char *path) override { Guard g; return o_->vol_.exists(path); }
  bool rename(const char *a, const char *b) override { Guard g; return o_->vol_.rename(a, b); }
  bool remove(const char *path) override { Guard g; return o_->vol_.remove(path); }
  bool mkdir(const char *path) override { Guard g; return o_->vol_.mkdir(path); }
  bool rmdir(const char *path) override { Guard g; return o_->vol_.rmdir(path); }
private:
  SdFatFS *o_;
};

SdFatFS::SdFatFS() : fs::FS(std::make_shared<SdFatFSImpl>(this)) {}

bool SdFatFS::setPins(int clk, int cmd, int d0) { clk_ = clk; cmd_ = cmd; d0_ = d0; return true; }

bool SdFatFS::begin(const char *, bool, bool) {
  if (clk_ < 0) return false;
  if (!g_lock) g_lock = xSemaphoreCreateRecursiveMutex();
  Guard g;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.width = 4;
  slot.clk = (gpio_num_t)clk_; slot.cmd = (gpio_num_t)cmd_; slot.d0 = (gpio_num_t)d0_;
  slot.d1 = (gpio_num_t)SDMMC_D1; slot.d2 = (gpio_num_t)SDMMC_D2; slot.d3 = (gpio_num_t)SDMMC_D3;
  esp_err_t e = sdmmc_host_init();
  if (e == ESP_OK) e = sdmmc_host_init_slot(host.slot, &slot);
  if (e == ESP_OK) e = sdmmc_card_init(&host, &g_card);
  if (e != ESP_OK) {
    Serial.printf("SD (SDMMC): card init failed 0x%x (a card that was in SPI mode needs a power cycle: take it out and put it back)\n", (unsigned)e);
    return false;
  }
  cardOk_ = true;
  if (!vol_.begin(&g_dev, true, 1)) { Serial.println("SD: no FAT / exFAT volume on the card (partition 1)"); return false; }
  return true;
}
uint32_t SdFatFS::sectors() { return cardOk_ ? g_card.csd.capacity : 0; }
uint64_t SdFatFS::cardSize() { return (uint64_t)sectors() * 512ULL; }
uint64_t SdFatFS::totalBytes() { Guard g; return (uint64_t)vol_.clusterCount() * vol_.bytesPerCluster(); }
uint64_t SdFatFS::usedBytes() { Guard g; return totalBytes() - (uint64_t)vol_.freeClusterCount() * vol_.bytesPerCluster(); }

SdFatFS SD_MMC;
#endif
