// CS43131 headphone DAC driver of the Fake Pod Nano Pro, from the JukeDial / TamaPoke-FakePod projects (LGPL-2.1-or-later, same author). Only built for that board.
#if defined(BOARD_FAKEPOD_NANO_PRO)
// SPDX-License-Identifier: LGPL-2.1-or-later
#include "bsp/bsp_dac.h"
#include <stdio.h>
#include "bsp/bsp_pins.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_attr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "pc/pc_cs43131.h"

static const char *TAG = "bsp_dac";
/* PSRAM (no internal RAM for these: none is touched from an ISR or with the cache off) */
static EXT_RAM_BSS_ATTR i2c_master_bus_handle_t s_bus;
static EXT_RAM_BSS_ATTR i2c_master_dev_handle_t s_dev;
static EXT_RAM_BSS_ATTR SemaphoreHandle_t s_lock;
static EXT_RAM_BSS_ATTR pc_cs_t s_cs;
static EXT_RAM_BSS_ATTR volatile bool s_ready;                       /* probe + boot done (internal RAM: a flag read from several tasks) */
static EXT_RAM_BSS_ATTR int s_last_err;
static EXT_RAM_BSS_ATTR bool s_shutdown_hooked;
static EXT_RAM_BSS_ATTR int s_cfg_hpf, s_cfg_fs, s_cfg_amute;
static EXT_RAM_BSS_ATTR int s_lab_pdn, s_lab_inv, s_lab_ibias;   /* LAB (bsp_lab.h): the 0x080046 pair, the detect polarity, XTAL_IBIAS (0 = the constant); read at the chip boot */
static EXT_RAM_BSS_ATTR int s_vol;   /* DACFS: dac_hpf / dac_fs / dac_amute (bsp_dac_config, before init) and the volume the audio layer last sent (applied at init) */
static EXT_RAM_BSS_ATTR bool s_cfg_set;

/* register address: 3 bytes high first, a control byte (0 = single 8-bit access), then the data. A read is the datasheet's two framings (Fig. 4-34): the address with a stop, then a read.
 * If the first probe gets no answer, the kernel driver's repeated-start form is the other thing to try (checklist). */
static int io_wr(void *ctx, uint32_t reg, uint8_t v)
{
    (void)ctx;
    const uint8_t b[5] = { (uint8_t)(reg >> 16), (uint8_t)(reg >> 8), (uint8_t)reg, 0x00, v };
    return i2c_master_transmit(s_dev, b, sizeof b, 30) == ESP_OK ? 0 : -1;
}
static int io_rd(void *ctx, uint32_t reg, uint8_t *v)
{
    (void)ctx;
    const uint8_t a[4] = { (uint8_t)(reg >> 16), (uint8_t)(reg >> 8), (uint8_t)reg, 0x00 };
    if (i2c_master_transmit(s_dev, a, sizeof a, 30) != ESP_OK) return -1;
    return i2c_master_receive(s_dev, v, 1, 30) == ESP_OK ? 0 : -1;
}
static void io_delay(void *ctx, uint32_t ms) { (void)ctx; vTaskDelay(pdMS_TO_TICKS(ms) ? pdMS_TO_TICKS(ms) : 1); }
static void io_reset(void *ctx, bool high) { (void)ctx; gpio_set_level(BSP_PIN_DAC_RST, high ? 1 : 0); }

static esp_err_t rc_to_err(int rc)
{
    s_last_err = rc;
    switch (rc) {
    case PC_CS_OK: return ESP_OK;
    case PC_CS_ERATE: return ESP_ERR_NOT_SUPPORTED;
    case PC_CS_ESTATE: return ESP_ERR_INVALID_STATE;
    case PC_CS_EID: return ESP_ERR_NOT_FOUND;
    case PC_CS_EXTAL: return ESP_ERR_TIMEOUT;
    default: return ESP_FAIL;
    }
}

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;   /* s_ready / s_vol / s_note: read and written from several tasks (internal RAM, a few bytes) */
/* FakePod I1: every restart (a mode change, esp_restart, a panic that reaches the handlers) stops the DAC the pop-free way while the I2S clocks still run (pc_cs_stop mutes, ramps,
 * powers the amplifier and the serial port down), then holds RESET low, so the next boot starts from a quiet part. The lock is tried for a short time only (a restart must not wait on a stuck I2C). */
static void dac_shutdown(void)
{
    if (!s_ready) return;
    const bool locked = s_lock && xSemaphoreTake(s_lock, pdMS_TO_TICKS(100)) == pdTRUE;
    (void)locked;
    pc_cs_shutdown(&s_cs);                               /* stop, crystal off, RESET low (held) */
    gpio_set_level(BSP_PIN_DAC_RST, 0);
    portENTER_CRITICAL(&s_mux); s_ready = false; portEXIT_CRITICAL(&s_mux);   /* a power-off that did not happen: the next start runs the whole probe + boot + mute order again */
    if (locked) xSemaphoreGive(s_lock);
}
void bsp_dac_shutdown(void) { dac_shutdown(); }

static EXT_RAM_BSS_ATTR volatile bool s_fault;
static EXT_RAM_BSS_ATTR volatile uint8_t s_note;
static EXT_RAM_BSS_ATTR int64_t s_fault_us;
static EXT_RAM_BSS_ATTR volatile bool s_absent, s_ever_ready;   /* A6: no CS43131 ever answered / one did once */

static bool bad_rc(int rc) { return rc == PC_CS_EIO || rc == PC_CS_EVOL || rc == PC_CS_ERESET || rc == PC_CS_EID; }

/* A DAC that stopped answering or lost its registers (lock held): RESET low at once (the pin does not need the bus; PDN_HP = 1 after a reset, the outputs are clamped), the part is
 * "not ready" until the normal probe + boot + mute order runs again (bsp_dac_init, at most every 5 s). The audio service also drops its unity digital gain to 0 while this lasts. */
static void dac_fault(const char *why, int rc)
{
    gpio_set_level(BSP_PIN_DAC_RST, 0);
    portENTER_CRITICAL(&s_mux);
    s_ready = false; s_fault = true; s_note = BSP_DAC_NOTE_DEAD;
    portEXIT_CRITICAL(&s_mux);
    s_fault_us = esp_timer_get_time();
    printf("APP dac FAULT %s rc=%d: RESET low, muted until re-initialised\n", why, rc);
}

static int vol_now(void) { portENTER_CRITICAL(&s_mux); int v = s_vol; portEXIT_CRITICAL(&s_mux); return v; }

/* reset, probe, boot (lock held, bus present); applies cfg and the volume the audio layer last sent */
static esp_err_t chip_init_locked(void)
{
    const pc_cs_io_t io = { NULL, io_wr, io_rd, io_delay, io_reset };
    pc_cs_init(&s_cs, &io);
    if (s_cfg_set) pc_cs_set_cfg(&s_cs, s_cfg_hpf, s_cfg_fs, s_cfg_amute);
    pc_cs_set_pdn_pair(&s_cs, BSP_DAC_PDN_PAIR || s_lab_pdn);
    pc_cs_set_lab(&s_cs, s_lab_inv, s_lab_ibias);
    int rc = pc_cs_probe(&s_cs);
    if ((rc == PC_CS_EIO || rc == PC_CS_EID) && !s_ever_ready) {   /* nothing (or another chip) at 0x30: say it, once, instead of failing silently. A PCM5102 variant of the FakePod has no I2C DAC at all */
        if (!s_absent) { portENTER_CRITICAL(&s_mux); s_absent = true; s_note = BSP_DAC_NOTE_ABSENT; portEXIT_CRITICAL(&s_mux); }
        printf("APP dac ABSENT: no CS43131 answered at 0x%02x (rc=%d; a PCM5102 variant has no I2C DAC): wired output refused, nothing is played through I2S\n", BSP_I2C_ADDR_DAC, rc);
    }
    if (rc == PC_CS_OK) rc = pc_cs_boot(&s_cs);
    esp_err_t e = rc_to_err(rc);
    bool ok = rc == PC_CS_OK;
    portENTER_CRITICAL(&s_mux);
    s_ready = ok;
    if (ok) { s_fault = false; s_ever_ready = true; s_absent = false; }
    portEXIT_CRITICAL(&s_mux);
    if (ok) { const int vr = pc_cs_set_volume(&s_cs, vol_now()); if (vr) printf("APP dac volume write rc=%d (stays muted)\n", vr); }   /* the volume before any unmute can happen */
    else s_fault_us = esp_timer_get_time();
    if (ok && !s_shutdown_hooked) s_shutdown_hooked = esp_register_shutdown_handler(dac_shutdown) == ESP_OK;
    printf("APP dac init rc=%d (%s)\n", rc, ok ? "ready, muted" : "FAILED");
    return e;
}

esp_err_t bsp_dac_init(void)
{
    if (!s_lock) s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    esp_err_t e = ESP_OK;
    if (s_ready) goto out;
    if (s_fault && esp_timer_get_time() - s_fault_us < 5000000) { e = ESP_ERR_INVALID_STATE; goto out; }   /* a faulted part is tried again every 5 s at most (no 1 s loop on a dead chip) */
    if (!s_bus) {
        /* RESET first: low until the driver pulses it (the board's pull-up would release the chip as soon as the rail is up) */
        const gpio_config_t g = { .pin_bit_mask = 1ULL << BSP_PIN_DAC_RST, .mode = GPIO_MODE_OUTPUT };
        e = gpio_config(&g);
        if (e != ESP_OK) goto out;
        gpio_set_level(BSP_PIN_DAC_RST, 0);
        const i2c_master_bus_config_t bc = {
            .i2c_port = BSP_I2C_PORT_DAC, .sda_io_num = BSP_PIN_DAC_SDA, .scl_io_num = BSP_PIN_DAC_SCL,
            .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7, .flags.enable_internal_pullup = 0,   /* 2.2 k pull-ups to 1.8 V on the board, behind the level shifter */
        };
        e = i2c_new_master_bus(&bc, &s_bus);
        if (e != ESP_OK) { ESP_LOGE(TAG, "i2c1 bus: %s", esp_err_to_name(e)); goto out; }
        const i2c_device_config_t dc = { .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = BSP_I2C_ADDR_DAC, .scl_speed_hz = BSP_I2C_FREQ_DAC_HZ };
        e = i2c_master_bus_add_device(s_bus, &dc, &s_dev);
        if (e != ESP_OK) { ESP_LOGE(TAG, "i2c1 device: %s", esp_err_to_name(e)); i2c_del_master_bus(s_bus); s_bus = NULL; goto out; }
    }
    e = chip_init_locked();
out:
    xSemaphoreGive(s_lock);
    return e;
}

static EXT_RAM_BSS_ATTR volatile bool s_cfg_dirty;   /* BTAUDIO: dac_hpf / dac_fs / dac_amute changed after the chip was booted: the next start resets and boots it again with them */
esp_err_t bsp_dac_start(uint32_t rate, uint8_t bits)
{
    if (s_cfg_dirty && s_ready && s_lock) {              /* quiet reset first (pc_cs_stop is the pop-free power-down), then the normal probe + boot below applies the new options */
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_cfg_dirty = false;
        pc_cs_stop(&s_cs);
        gpio_set_level(BSP_PIN_DAC_RST, 0);
        portENTER_CRITICAL(&s_mux); s_ready = false; portEXIT_CRITICAL(&s_mux);
        xSemaphoreGive(s_lock);
        printf("APP dac options changed: reset and boot again\n");
    }
    if (!s_ready) { esp_err_t ie = bsp_dac_init(); if (ie != ESP_OK) return ESP_ERR_INVALID_STATE; }   /* a faulted part: the whole probe + boot first */
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!s_ready) { xSemaphoreGive(s_lock); return ESP_ERR_INVALID_STATE; }
    s_cs.vol = vol_now();                                /* the latest volume (a set_volume that found the lock busy only stored it) */
    int rc = pc_cs_start(&s_cs, rate, bits);
    if (rc == PC_CS_ERESET) {                            /* the chip lost its registers: reset, boot, once more (never a power-up on the defaults) */
        printf("APP dac registers lost (reset / brown-out): re-initialising\n");
        gpio_set_level(BSP_PIN_DAC_RST, 0);
        portENTER_CRITICAL(&s_mux); s_ready = false; portEXIT_CRITICAL(&s_mux);
        if (chip_init_locked() == ESP_OK) rc = pc_cs_start(&s_cs, rate, bits);
    }
    if (rc == PC_CS_ERATE && rate) printf("APP dac rate %lu has no ASP_SPRATE code: refused\n", (unsigned long)rate);
    else if (rc == PC_CS_EVOL) printf("APP dac volume registers did not read back at %lu Hz: stays muted\n", (unsigned long)rate);
    else if (rc) printf("APP dac start rate=%lu bits=%u rc=%d\n", (unsigned long)rate, (unsigned)bits, rc);
    if (rc == PC_CS_OK) {
        if (s_cs.note == PC_CS_NOTE_PLL) { s_cs.note = 0; s_note = BSP_DAC_NOTE_PLL; printf("APP dac PLL did not lock: %lu Hz plays from the 24.576 MHz crystal\n", (unsigned long)rate); }
        const int v = vol_now();
        if (v != s_cs.vol) pc_cs_set_volume(&s_cs, v);   /* changed while the start held the lock */
    } else if (bad_rc(rc)) dac_fault("start", rc);
    esp_err_t e = rc_to_err(rc);
    xSemaphoreGive(s_lock);
    return e;
}

bool bsp_dac_cfg_pending(void) { return s_cfg_dirty; }   /* FIX4 I1: an option changed and the chip still runs the old ones */

void bsp_dac_config(int hpf, int fs, int amute)
{
    if (s_cfg_set && (s_cfg_hpf != hpf || s_cfg_fs != fs || s_cfg_amute != amute)) s_cfg_dirty = true;   /* BTAUDIO: a live change (a boot-time call finds s_cfg_set false) */
    s_cfg_hpf = hpf; s_cfg_fs = fs; s_cfg_amute = amute; s_cfg_set = true;   /* read by bsp_dac_init (a boot-time setting) */
}

void bsp_lab_dac(int pdn_pair, int hp_inv, int xtal_ibias)
{
    const int ib = xtal_ibias == 4 ? 0 : xtal_ibias;   /* 4 is the constant's own value */
    if (s_cfg_set && (s_lab_pdn != pdn_pair || s_lab_inv != hp_inv || s_lab_ibias != ib)) s_cfg_dirty = true;   /* a live change: the next start resets the chip and boots it with them (as dac_fs) */
    s_lab_pdn = pdn_pair; s_lab_inv = hp_inv; s_lab_ibias = ib;
}

esp_err_t bsp_dac_set_volume(int vol)
{
    portENTER_CRITICAL(&s_mux);
    s_vol = vol;
    const bool ready = s_ready, fault = s_fault;
    portEXIT_CRITICAL(&s_mux);
    if (!ready) return fault ? ESP_ERR_INVALID_STATE : ESP_OK;   /* applied by bsp_dac_init; a faulted part reports it (the audio layer goes digital-silent) */
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(20)) != pdTRUE) return ESP_OK;   /* a rate switch holds the bus (~200 ms): the value is stored and written when it ends; the UI never waits for it */
    esp_err_t e = ESP_OK;
    if (s_ready) {
        int rc = pc_cs_set_volume(&s_cs, vol);
        if (rc) printf("APP dac set volume %d rc=%d (muted)\n", vol, rc);
        if (bad_rc(rc)) dac_fault("volume", rc);
        e = rc_to_err(rc);
    }
    xSemaphoreGive(s_lock);
    return e;
}

esp_err_t bsp_dac_stop(void)
{
    if (!s_ready) return ESP_OK;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    int rc = pc_cs_stop(&s_cs);
    if (rc) printf("APP dac stop rc=%d\n", rc);
    if (bad_rc(rc)) dac_fault("stop", rc);               /* an answer-less chip after the power-down: reset it, the next start re-initialises */
    esp_err_t e = rc_to_err(rc);
    xSemaphoreGive(s_lock);
    return e;
}

esp_err_t bsp_dac_mute(bool mute)
{
    if (!s_ready) return mute ? ESP_OK : ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    int rc = pc_cs_mute(&s_cs, mute);
    if (bad_rc(rc)) dac_fault("mute", rc);
    esp_err_t e = rc_to_err(rc);
    xSemaphoreGive(s_lock);
    return e;
}

int bsp_dac_serial_errors(void)
{
    if (!s_ready) return 0;
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(20)) != pdTRUE) return 0;
    int r = s_ready ? pc_cs_serial_errors(&s_cs) : 0;
    xSemaphoreGive(s_lock);
    return r > 0 ? r : 0;
}

uint8_t bsp_dac_take_note(void)
{
    portENTER_CRITICAL(&s_mux);
    const uint8_t n = s_note;
    s_note = 0;
    portEXIT_CRITICAL(&s_mux);
    return n;
}

bool bsp_dac_fault(void) { return s_fault; }
bool bsp_dac_absent(void) { return s_absent; }

int bsp_dac_headphone(void)
{
    if (!s_ready) return -1;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    int r = pc_cs_headphone(&s_cs);
    xSemaphoreGive(s_lock);
    return r;
}

int bsp_dac_drive(void) { return s_ready ? pc_cs_drive_rb(&s_cs) : -1; }
int bsp_dac_drive_set(void) { return s_cs.cfg_fs; }

void bsp_dac_dump(void)
{
    uint8_t hp = 0, ch = 0;
    char hs[8] = "n/a", cs[8] = "n/a";   /* the registers as the chip holds them now ("n/a": not readable) */
    if (s_ready) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        if (pc_cs_read_drive_regs(&s_cs, &hp, &ch) == PC_CS_OK) { snprintf(hs, sizeof hs, "0x%02X", hp); snprintf(cs, sizeof cs, "0x%02X", ch); }
        xSemaphoreGive(s_lock);
    }
    printf("APP dac absent=%d ready=%d state=%d rate=%lu bits=%u muted=%d vol=%d vol_ok=%d clk=%d hp=%d last_rc=%d fs=%d classh=%s hp_out=%s drive=%d\n", s_absent, s_ready, s_cs.state, (unsigned long)s_cs.rate, (unsigned)s_cs.bits, s_cs.muted,
           s_cs.vol, s_cs.vol_ok, s_cs.clk, bsp_dac_headphone(), s_last_err, s_cs.cfg_fs, cs, hs, s_cs.drive_rb);
}
#endif  // BOARD_FAKEPOD_NANO_PRO
