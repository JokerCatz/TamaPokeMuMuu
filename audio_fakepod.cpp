// Fake Pod Nano Pro sound: the game's square-wave effects through the CS43131 headphone DAC (the board has no speaker). Replaces audio.cpp (ES8311 + speaker, Waveshare).
// From the TamaPoke-FakePod project (same board, same driver, verified there). Only built for that board.
#if defined(BOARD_FAKEPOD_NANO_PRO)
// TamaPoke's sound effects (the square-wave jingles of upstream audio.cpp) through the Fake Pod's CS43131 DAC (headphone output only: the board has no speaker).
// The DAC driver is the JukeDial project's (bsp_dac.c + pc_cs43131.c, in fakepod_bsp): I2S master -> DAC slave at 48 kHz, 16-bit data, the DAC's own digital volume.
//
// AUDIO SAFETY: the DAC is driven directly into headphones, and a sensitive in-ear monitor turns a square wave at -16 dBFS into a very loud sound. The DAC volume is therefore fixed
// low (FP_DAC_VOLUME) and the game's own three levels (sfxAmp: 5000 / 2000 / 700) scale below that. The DAC stays muted until the first effect, and while the screen is asleep.
#include "audio.h"
#include <Arduino.h>
#include <Preferences.h>
extern "C" {
#include "bsp/bsp_dac.h"   // a C header without its own extern "C" guard
}
#include "bsp/bsp_pins.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define SAMPLE_RATE 48000
#define FP_DAC_VOLUME 45     /* 0..100 on the DAC's scale (100 = 0 dB, 10 = -54 dB, 0.6 dB per step): 45 = -33 dB. Raise only after listening. */
static const char *TAG = "audio";

static i2s_chan_handle_t s_tx;
static bool gReady = false, gOn = true, gSleeping = false;
static uint8_t gVol = 0;                // audio.h: 0 high, 1 medium, 2 low
static QueueHandle_t gQ = nullptr;
static volatile bool gDacMuted = true;

// ---- the effects (upstream audio.cpp) ----
struct Note { uint16_t f, ms; };
static const Note N_TAP[]    = {{880, 35}};
static const Note N_EAT[]    = {{660, 45}, {0, 12}, {660, 45}};
static const Note N_PLAY[]   = {{784, 45}, {988, 60}};
static const Note N_HEART[]  = {{1047, 55}, {1319, 90}};
static const Note N_HATCH[]  = {{523, 80}, {659, 80}, {784, 110}, {1047, 170}};
static const Note N_EVOLVE[] = {{523, 80}, {659, 80}, {784, 80}, {1047, 90}, {1319, 230}};
static const Note N_MEDAL[]  = {{784, 70}, {0, 25}, {784, 70}, {0, 25}, {1047, 200}};
static const Note N_DENY[]   = {{300, 110}, {200, 170}};
static const Note N_BYE[]    = {{784, 150}, {659, 150}, {523, 280}};
static const Note N_LEVEL[]  = {{784, 70}, {1047, 130}};
struct SfxDef { const Note *n; uint8_t len; };
static const SfxDef SFX[SFX_COUNT] = {
  {N_TAP, 1}, {N_EAT, 3}, {N_PLAY, 2}, {N_HEART, 2}, {N_HATCH, 4},
  {N_EVOLVE, 5}, {N_MEDAL, 5}, {N_DENY, 2}, {N_BYE, 3}, {N_LEVEL, 2},
};

static void applyMute()
{
  const bool want = !(gReady && gOn && !gSleeping);
  if (want == gDacMuted) return;
  bsp_dac_mute(want);
  gDacMuted = want;
}

static int16_t buf[256 * 2];           // interleaved stereo (L = R)
static void writeFrames(int frames)
{
  size_t w = 0;
  i2s_channel_write(s_tx, buf, (size_t)frames * 4, &w, 200);
}

// one tone (silence when f == 0), with an attack / release ramp against clicks
static void playTone(uint16_t f, uint16_t ms)
{
  const int total = SAMPLE_RATE * ms / 1000;
  const int half = f ? (SAMPLE_RATE / (2 * f)) : 0;
  const int16_t amp = sfxAmp(gVol);
  const int ramp_in = SAMPLE_RATE / 250, ramp_out = SAMPLE_RATE / 160;   // the upstream 64 / 96 samples at 16 kHz, scaled to 48 kHz
  int phase = 0, done = 0;
  bool high = true;
  while (done < total) {
    int n = total - done; if (n > 256) n = 256;
    for (int i = 0; i < n; i++) {
      int16_t s = 0;
      if (f) {
        s = high ? amp : -amp;
        const int idx = done + i;
        if (idx < ramp_in) s = (int16_t)(s * idx / ramp_in);
        else if (idx > total - ramp_out) s = (int16_t)(s * (total - idx) / ramp_out);
        if (++phase >= half) { phase = 0; high = !high; }
      }
      buf[i * 2] = s; buf[i * 2 + 1] = s;
    }
    writeFrames(n);
    done += n;
  }
}

static void audioTask(void *)
{
  uint8_t id;
  for (;;) {
    if (xQueueReceive(gQ, &id, portMAX_DELAY) && gOn && !gSleeping && gReady && id < SFX_COUNT) {
      applyMute();                       // unmute on the first effect (the DAC volume is already written and read back)
      const SfxDef &d = SFX[id];
      for (uint8_t i = 0; i < d.len; i++) playTone(d.n[i].f, d.n[i].ms);
    }
  }
}

void audioBegin()
{
  Preferences p;
  p.begin("tamapoke", true);
  gOn = p.getBool("snd", true);
  gVol = p.getUChar("vol", 0) % VOL_LEVELS;
  p.end();

  bsp_dac_set_volume(FP_DAC_VOLUME);     // stored until the chip is initialised; written and read back before any unmute
  if (bsp_dac_init() != ESP_OK) { ESP_LOGW(TAG, "CS43131 not ready: sound off"); return; }

  i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(BSP_I2S_PORT_DAC, I2S_ROLE_MASTER);
  cc.dma_desc_num = 6; cc.dma_frame_num = 480;
  cc.auto_clear = true;                  // an underrun plays silence, not stale data
  if (i2s_new_channel(&cc, &s_tx, NULL) != ESP_OK) { ESP_LOGW(TAG, "i2s channel failed"); return; }
  i2s_std_config_t sc = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = { .mclk = GPIO_NUM_NC, .bclk = BSP_PIN_I2S_BCK, .ws = BSP_PIN_I2S_WS, .dout = BSP_PIN_I2S_DOUT, .din = GPIO_NUM_NC },
  };
  sc.clk_cfg.clk_src = I2S_CLK_SRC_XTAL;
  if (i2s_channel_init_std_mode(s_tx, &sc) != ESP_OK || i2s_channel_enable(s_tx) != ESP_OK) { ESP_LOGW(TAG, "i2s init failed"); i2s_del_channel(s_tx); s_tx = nullptr; return; }
  if (bsp_dac_start(SAMPLE_RATE, 16) != ESP_OK) { ESP_LOGW(TAG, "DAC refused the clocks: sound off"); i2s_channel_disable(s_tx); i2s_del_channel(s_tx); s_tx = nullptr; return; }
  gReady = true;
  gQ = xQueueCreate(8, sizeof(uint8_t));
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 3, nullptr, 0);
  ESP_LOGI(TAG, "sound ready: CS43131, %d Hz, DAC volume %d, %s", SAMPLE_RATE, FP_DAC_VOLUME, gOn ? "on" : "muted by the setting");
  bsp_dac_dump();
}

void sfxPlay(uint8_t id)
{
  if (gReady && gOn && !gSleeping && gQ) xQueueSend(gQ, &id, 0);   // dropped when the queue is full
}
void audioSetEnabled(bool on)
{
  gOn = on; applyMute();
  Preferences p; p.begin("tamapoke", false); p.putBool("snd", on); p.end();
}
bool audioEnabled() { return gOn; }
void audioSetVolume(uint8_t level)
{
  gVol = level % VOL_LEVELS;
  Preferences p; p.begin("tamapoke", false); p.putUChar("vol", gVol); p.end();
}
uint8_t audioVolume() { return gVol; }
void audioSetSleeping(bool sleeping) { if (gSleeping != sleeping) { gSleeping = sleeping; applyMute(); } }
bool audioBusy() { return false; }
void audioSetIdle(bool) {}
#endif  // BOARD_FAKEPOD_NANO_PRO
