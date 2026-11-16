#include "photos.h"
#include "sdmon.h"
#include <FS.h>
#include <SD_MMC.h>

#define PHOTO_PATH "/friends.bin"

bool photoAdd(const FriendPhoto &p) {
  if (!sdReady) return false;
  File f = SD_MMC.open(PHOTO_PATH, FILE_APPEND);
  if (!f) return false;
  bool ok = f.write((const uint8_t *)&p, sizeof(p)) == sizeof(p);
  f.close();
  return ok;
}

int photoCount() {
  if (!sdReady) return 0;
  File f = SD_MMC.open(PHOTO_PATH, FILE_READ);
  if (!f) return 0;
  int n = (int)(f.size() / sizeof(FriendPhoto));
  f.close();
  return n;
}

bool photoGet(int i, FriendPhoto &p) {
  int n = photoCount();
  if (i < 0 || i >= n) return false;
  File f = SD_MMC.open(PHOTO_PATH, FILE_READ);
  if (!f) return false;
  bool ok = f.seek((uint32_t)(n - 1 - i) * sizeof(FriendPhoto)) && f.read((uint8_t *)&p, sizeof(p)) == sizeof(p) &&
            p.ver == PHOTO_VER;
  p.name[LINK_NAME] = 0;
  f.close();
  return ok;
}
