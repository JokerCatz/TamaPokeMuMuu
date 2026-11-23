// Fake Pod Nano Pro: the PCF85063 RTC, the AXP2101's battery / USB facts and the two side keys (replaces rtcbat.cpp, which is for the Waveshare board).
// Ported from the TamaPoke-FakePod / JukeDial projects (same board, verified there). Rules kept from those:
//  - the AXP2101 is READ-ONLY here: no voltage, rail, charger, ADC-enable or power-key-timing register is ever written (the AMOLED supply is the GPIO LCD_VCI_EN, not a PMU rail).
//    The only writes: the key-IRQ enable register 0x41, the write-1-to-clear of IRQ status bits that were read, and the soft power-off bit (0x10 bit 0) in batPowerOff.
//  - the RTC chip keeps UTC (the JukeDial project shares it); the game's wall time comes from it through wallFromUtc() (the time zone of config.h / the city offset).
#if defined(BOARD_FAKEPOD_NANO_PRO)
#include "batcurve.h"
#include "rtcbat.h"
#include "power.h"
#include "walltime.h"
#include "config.h"      // TZ_STD_OFFSET_H
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <time.h>

// ---------------------------------------------------------------- I2C helpers
static bool i2cRead(uint8_t addr, uint8_t reg, uint8_t *buf, size_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, (int)n) != (int)n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}
static bool i2cWrite(uint8_t addr, uint8_t reg, const uint8_t *buf, size_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  for (size_t i = 0; i < n; i++) Wire.write(buf[i]);
  return Wire.endTransmission() == 0;
}

// ---------------------------------------------------------------- RTC PCF85063 (0x51)
#define RTC_ADDR 0x51
static bool rtcOk = false;
static int bcd2(uint8_t v, int hiMask, int *out) { int t = (v & hiMask) >> 4, o = v & 15; if (t > 9 || o > 9) return 0; *out = t * 10 + o; return 1; }
static uint8_t toBcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static int64_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2; const int era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (unsigned)(m + (m > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}
static void civilFromDays(int64_t z, int *y, int *m, int *d) {
  z += 719468; const int64_t era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; int yy = (int)yoe + (int)era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153;
  *d = (int)(doy - (153 * mp + 2) / 5 + 1); *m = (int)(mp < 10 ? mp + 3 : mp - 9); if (*m <= 2) yy++; *y = yy;
}
static int dayOfWeek(int y, int m, int d) { static const int t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }; if (m < 3) y--; return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; }

bool rtcBegin() {
  uint8_t c1 = 0;
  rtcOk = i2cRead(RTC_ADDR, 0x00, &c1, 1);
  if (!rtcOk) { Serial.println("PCF85063 no detectado"); return false; }
  const uint8_t run = c1 & ~(0x20 | 0x02);  // STOP and 12_24 cleared, CAP_SEL / CIE kept; written only when needed
  if (run != c1) { i2cWrite(RTC_ADDR, 0x00, &run, 1); Serial.printf("RTC ctrl1 %02x -> %02x\n", c1, run); }
  return true;
}

// the chip's time as UTC seconds; 0 when the chip has no valid time
static uint32_t chipUtc() {
  if (!rtcOk) return 0;
  uint8_t r[7];
  if (!i2cRead(RTC_ADDR, 0x04, r, 7)) return 0;
  if (r[0] & 0x80) return 0;  // oscillator stopped: not trustworthy
  int s, mi, h, d, mo, y;
  if (!bcd2(r[0], 0x70, &s) || !bcd2(r[1], 0x70, &mi) || !bcd2(r[2], 0x30, &h) || !bcd2(r[3], 0x30, &d) || !bcd2(r[5], 0x10, &mo) || !bcd2(r[6], 0xF0, &y)) return 0;
  if ((r[1] & 0x80) || (r[2] & 0xC0) || s > 59 || mi > 59 || h > 23 || mo < 1 || mo > 12 || d < 1 || d > 31) return 0;
  const int year = 2000 + y;
  if (year < 2025 || year > 2099) return 0;
  const int64_t e = daysFromCivil(year, mo, d) * 86400 + h * 3600 + mi * 60 + s;
  return e > 0 ? (uint32_t)e : 0;
}

uint32_t rtcEpoch() {  // the game's wall time (local), 0 = no valid time
  const uint32_t u = chipUtc();
  return u ? wallFromUtc(u) : 0;
}

void rtcSetEpoch(uint32_t wall) {  // wall time in, the chip gets UTC
  if (!rtcOk) return;
  uint32_t guess = wall - (uint32_t)((int32_t)TZ_STD_OFFSET_H * 3600);  // standard-time guess, then corrected by what the zone really adds at that moment
  const int32_t off = (int32_t)(wallFromUtc(guess) - guess);
  const uint32_t utc = wall - (uint32_t)off;
  int64_t days = utc / 86400; const int rem = (int)(utc % 86400);
  int y, m, d; civilFromDays(days, &y, &m, &d);
  if (y < 2000 || y > 2099) return;
  uint8_t r[7] = { toBcd(rem % 60), toBcd(rem % 3600 / 60), toBcd(rem / 3600), toBcd(d), (uint8_t)dayOfWeek(y, m, d), toBcd(m), toBcd(y - 2000) };
  if (!i2cWrite(RTC_ADDR, 0x04, r, 7)) Serial.println("RTC: set failed");  // seconds bit 7 = 0 also clears the oscillator-stop flag
}

// ---------------------------------------------------------------- AXP2101 (0x34), reads only
#define PMU_ADDR 0x34
static bool pmuOk = false;
static uint32_t powerCacheT = 0, pollMs = 2000;
static int cachedPct = -1, cachedMv = 0;
static bool cachedCharging = false, cachedUsb = true, monDischarging = false;
static BatMonitor mon;

// 600 mAh LCO open-circuit table + gauge fusion (NightPlayer (c) 2026 ZYFDroid, MIT; via the JukeDial project)
static int pctFromMv(int mv) {
  static const struct { int mv, pct; } T[] = { { 3300, 0 }, { 3420, 3 }, { 3490, 5 }, { 3570, 10 }, { 3630, 15 }, { 3680, 20 }, { 3740, 30 }, { 3800, 40 }, { 3860, 50 },
                                               { 3920, 60 }, { 3980, 70 }, { 4050, 80 }, { 4110, 90 }, { 4150, 95 }, { 4200, 100 } };
  const int n = (int)(sizeof T / sizeof T[0]);
  if (mv <= T[0].mv) return 0;
  if (mv >= T[n - 1].mv) return 100;
  for (int i = 1; i < n; i++) if (mv < T[i].mv) return T[i - 1].pct + (T[i].pct - T[i - 1].pct) * (mv - T[i - 1].mv) / (T[i].mv - T[i - 1].mv);
  return 100;
}
static int fuse(int gauge, int mv, bool charging, bool done) {
  const bool vok = mv >= 2500 && mv < 4600;
  const int ocv = vok ? pctFromMv(mv) : -1;
  if (gauge > 100) gauge = -1;
  if (done) return 100;
  if (charging) { const int g = gauge >= 0 ? gauge : ocv; return g < 0 ? -1 : (g > 99 ? 99 : g); }
  if (gauge < 0) return ocv;
  if (ocv < 0) return gauge;
  const bool suspicious = (gauge == 0 && mv > 3600) || (gauge == 100 && mv < 4000);
  const int diff = gauge > ocv ? gauge - ocv : ocv - gauge;
  if (suspicious || diff > 8) return ocv;
  return (6 * gauge + 4 * ocv + 5) / 10;
}
struct PmuRead { bool ok, vbus, batt, charging, done; int mv, pct; };
static PmuRead pmuRead() {
  PmuRead p = {};
  uint8_t s1, s2, vh, vl, g;
  if (!pmuOk || !i2cRead(PMU_ADDR, 0x00, &s1, 1) || !i2cRead(PMU_ADDR, 0x01, &s2, 1) || !i2cRead(PMU_ADDR, 0x34, &vh, 1) || !i2cRead(PMU_ADDR, 0x35, &vl, 1) || !i2cRead(PMU_ADDR, 0xA4, &g, 1)) return p;
  p.ok = true;
  p.vbus = (s1 & 0x20) != 0;
  p.batt = (s1 & 0x08) != 0;
  const uint8_t dir = (s2 >> 5) & 3, cs = s2 & 7;
  p.charging = dir == 1;
  p.done = p.vbus && cs == 4;
  p.mv = p.batt ? ((vh & 0x3F) << 8) | vl : 0;
  p.pct = p.batt ? fuse(g == 0xFF ? -1 : (g & 0x7F), p.mv, p.charging, p.done) : -1;
  return p;
}

bool batBegin() {
  uint8_t id = 0;
  pmuOk = i2cRead(PMU_ADDR, 0x03, &id, 1) && (id & 0xCF) == 0x4A;
  if (!pmuOk) Serial.println("AXP2101 no detectado");
  return pmuOk;
}

// The AMOLED supply is a GPIO on this board (not a PMU rail): a clean off/on cycle, then time for the rails before the panel's reset pulse
// (board day 2026-11-22: with a shorter wait 3 of 5 soft reboots left the panel showing its power-on RAM). Call before gfx->begin().
void pmuEnablePanel() {
  pinMode(LCD_VCI_EN, OUTPUT);
  digitalWrite(LCD_VCI_EN, LOW);
  delay(50);
  digitalWrite(LCD_VCI_EN, HIGH);
  delay(120);
}

void batSetPollMs(uint32_t ms) { pollMs = ms; }

static void refreshPower() {
  uint32_t now = millis();
  if (powerCacheT && now - powerCacheT < pollMs) return;
  powerCacheT = now ? now : 1;
  PmuRead p = pmuRead();
  if (!p.ok) { cachedPct = -1; cachedCharging = false; cachedUsb = true; cachedMv = 0; return; }
  cachedCharging = p.charging; cachedUsb = p.vbus; cachedMv = p.mv;
  // on battery the percentage comes from the discharge curve (batcurve.h); with USB or while charging the voltage is inflated, so the PMU's own figure is used
  bool discharging = p.batt && !cachedUsb && !cachedCharging;
  if (discharging != monDischarging) { mon.reset(); mon.seed(cachedPct); monDischarging = discharging; }
  if (discharging) cachedPct = mon.sample(p.mv, now);
  else cachedPct = p.batt ? p.pct : -1;
  if (cachedPct < 0 && p.batt) cachedPct = p.pct;
}

int batPercent() { refreshPower(); return cachedPct; }
bool batCharging() { refreshPower(); return cachedCharging; }
bool batShouldShutdown() { refreshPower(); return monDischarging && mon.shutdownNeeded(millis()); }
int batMillivolts() { refreshPower(); return cachedMv; }
bool usbPresent() { refreshPower(); return cachedUsb; }
bool usbPresentNow() { PmuRead p = pmuRead(); return p.ok ? p.vbus : true; }
void batPowerOff() {
  uint8_t v = 0;
  if (pmuOk && i2cRead(PMU_ADDR, 0x10, &v, 1)) { v |= 0x01; i2cWrite(PMU_ADDR, 0x10, &v, 1); }  // SOFT_PWROFF, read-modify-write
  mon.reset();  // still alive (USB plugged in just now): everything has to hold for the whole time again
}

// ---------------------------------------------------------------- the two side keys
// Top key = the AXP2101 power key: its edges latch in INTSTS2 (bit1 pressed, bit0 released) until cleared; nIRQ (GPIO49) goes low meanwhile. A long hold (4 s) is the PMU's own hard
// power-off and is not touched. Bottom key = BOOTMODE (GPIO61): an interrupt timestamps the edges so a quick tap is never missed between two polls.
static volatile uint32_t bottomDownUs = 0;
static volatile bool bottomShort = false;
static void IRAM_ATTR bottomIsr() {
  const uint32_t now = micros();
  if (digitalRead(KEY_BOTTOM_PIN) == LOW) bottomDownUs = now ? now : 1;
  else if (bottomDownUs) {
    const uint32_t d = now - bottomDownUs;
    if (d > 20000 && d < 1500000) bottomShort = true;  // 20 ms debounce, 1.5 s limit
    bottomDownUs = 0;
  }
}

void pwrSetup() {
  pinMode(KEY_BOTTOM_PIN, INPUT);  // a strapping pin: the board's 10k pull-up only
  attachInterrupt(digitalPinToInterrupt(KEY_BOTTOM_PIN), bottomIsr, CHANGE);
  pinMode(PMU_IRQ_PIN, INPUT);
  if (!pmuOk) return;
  uint8_t en = 0;
  if (i2cRead(PMU_ADDR, 0x41, &en, 1) && en != 0x0F) { const uint8_t v = 0x0F; i2cWrite(PMU_ADDR, 0x41, &v, 1); }  // power-key edges + short + long press, nothing else
  const uint8_t all = 0xFF;
  i2cWrite(PMU_ADDR, 0x49, &all, 1);  // the start-up's old events (the press that powered the device)
}

bool pwrShortPressed() {  // poll in the loop: true once per short press of either key
  bool hit = false;
  if (bottomShort) { bottomShort = false; hit = true; }
  if (pmuOk && digitalRead(PMU_IRQ_PIN) == LOW) {
    uint8_t st = 0;
    if (i2cRead(PMU_ADDR, 0x49, &st, 1) && st) {
      i2cWrite(PMU_ADDR, 0x49, &st, 1);  // write-1-to-clear only what was read
      if ((st & 0x03) == 0x03 || (st & 0x01)) hit = true;  // pressed and released (latched together), or a release
    }
    for (uint8_t reg = 0x48; reg <= 0x4A; reg += 2) {  // any other PMU event holding nIRQ low is read and cleared too: the line must go quiet
      uint8_t o = 0;
      if (i2cRead(PMU_ADDR, reg, &o, 1) && o) i2cWrite(PMU_ADDR, reg, &o, 1);
    }
  }
  return hit;
}
#endif  // BOARD_FAKEPOD_NANO_PRO
