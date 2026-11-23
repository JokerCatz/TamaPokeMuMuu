// Fake Pod Nano Pro sound: silent for now (the CS43131 headphone DAC comes next); same API as audio.h, with the settings kept in RAM / NVS like audio.cpp's no-codec branch.
#if defined(BOARD_FAKEPOD_NANO_PRO)
#include "audio.h"
#include <Preferences.h>
static bool gOn = true;
static uint8_t gVol = 0;
void audioBegin() {
  Preferences p;
  p.begin("tamapoke", true);
  gOn = p.getBool("snd", true);
  gVol = p.getUChar("vol", 0) % VOL_LEVELS;
  p.end();
}
void sfxPlay(uint8_t) {}
void audioSetEnabled(bool on) { gOn = on; Preferences p; p.begin("tamapoke", false); p.putBool("snd", on); p.end(); }
bool audioEnabled() { return gOn; }
void audioSetVolume(uint8_t level) { gVol = level % VOL_LEVELS; Preferences p; p.begin("tamapoke", false); p.putUChar("vol", gVol); p.end(); }
uint8_t audioVolume() { return gVol; }
void audioSetSleeping(bool) {}
bool audioBusy() { return false; }
void audioSetIdle(bool) {}
#endif
