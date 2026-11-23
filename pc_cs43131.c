// CS43131 headphone DAC driver of the Fake Pod Nano Pro, from the JukeDial / TamaPoke-FakePod projects (LGPL-2.1-or-later, same author). Only built for that board.
#if defined(BOARD_FAKEPOD_NANO_PRO)
// SPDX-License-Identifier: LGPL-2.1-or-later
#include "pc/pc_cs43131.h"
#include <string.h>

/* 24-bit register addresses [DS chapter 7; notes 4, 6, 9] */
#define R_DEVID_AB   0x010000u
#define R_DEVID_CD   0x010001u
#define R_DEVID_E    0x010002u
#define R_SYSCLK     0x010006u   /* MCLK_INT bit2 (0 = 24.576 MHz), MCLK_SRC_SEL bits1:0 (00 = the crystal directly) */
#define R_PLL_START  0x030001u
#define R_PLL_FRAC   0x030002u   /* three bytes: LSB, middle, MSB */
#define R_PLL_INT    0x030005u
#define R_PLL_OUTDIV 0x030008u
#define R_PLL_CAL    0x03000Au
#define R_PLL_MODE   0x03001Bu
#define R_PLL_PREDIV 0x040002u
#define R_SPRATE     0x01000Bu
#define R_SPSIZE     0x01000Cu
#define R_INT_ST2    0x0F0001u   /* ASP_NOLRCK bit3, ASP_EARLY bit4, ASP_LATE bit5, ASP_ERROR bit6 (DS 4.9.9 p.54), sticky: read to clear */
#define R_TEST_KEY   0x010010u   /* the datasheet's pop-free sequence uses these two test registers (Ex. 5-5) */
#define R_PDN        0x020000u   /* power down: bit3 XTAL, bit4 HP, bit6 ASP */
#define R_XTAL_IBIAS 0x020052u
#define R_LCHI       0x040014u
#define R_LCPR       0x040016u
#define R_ASP_CLK    0x040018u
#define R_ASP_FRAME  0x040019u
#define R_CH1_LOC    0x050000u
#define R_CH2_LOC    0x050001u
#define R_CH1_CFG    0x05000Au
#define R_CH2_CFG    0x05000Bu
#define R_HP_OUT1    0x080000u
#define R_VCPFILT    0x080046u   /* DS 5.8.1 test register (external VCPFILT mode): only with cfg_pdn_pair */
#define R_DXD13      0x080032u
#define R_PCM_FILTER 0x090000u   /* bit1 HIGH_PASS (reset 1), bit0 de-emphasis (0) */
#define R_PCM_VOL_B  0x090001u
#define R_PCM_VOL_A  0x090002u
#define R_PCM_CTL1   0x090003u   /* soft ramp, auto-mute kept; bit1 = mute A, bit0 = mute B */
#define R_CLASSH     0x0B0000u
#define R_HPDET_CTL  0x0D0000u
#define R_HPDET_ST   0x0D0001u   /* bit6 plug (debounced), bit5 unplug (debounced) */
#define R_INT_ST     0x0F0000u   /* sticky, read to clear: bit4 XTAL_READY, bit3 XTAL_ERROR, bit0 PDN_DONE */
#define R_INT_MASK   0x0F0010u

#define PDN_XTAL 0x08
#define PDN_PLL  0x04
#define PDN_HP   0x10
#define PDN_ASP  0x40
#define PCM_BEQA   0x40          /* PCM_VOL_BEQA: PCM_VOLUME_B follows PCM_VOLUME_A (one write, one read-back, both channels at once) */
#define PCM_BASE   (0xA8 | PCM_BEQA)   /* soft ramp, auto-mute (PCM_AMUTE bit3), BEQA */
#define PCM_AMUTE  0x08
#define INT_XTAL_READY 0x10
#define INT_XTAL_ERR   0x08
#define INT_PDN_DONE   0x01
#define INT_PLL_ERR    0x02
#define INT_PLL_READY  0x04

static int w(pc_cs_t *c, uint32_t r, uint8_t v) { return c->io.wr(c->io.ctx, r, v) ? PC_CS_EIO : PC_CS_OK; }
static int rd(pc_cs_t *c, uint32_t r, uint8_t *v) { return c->io.rd(c->io.ctx, r, v) ? PC_CS_EIO : PC_CS_OK; }
static void ms(pc_cs_t *c, uint32_t n) { c->io.delay_ms(c->io.ctx, n); }
#define TRY(x) do { int _e = (x); if (_e) return _e; } while (0)

static int rmw(pc_cs_t *c, uint32_t r, uint8_t clear, uint8_t set)
{
    uint8_t v = 0;
    TRY(rd(c, r, &v));
    return w(c, r, (uint8_t)((v & (uint8_t)~clear) | set));
}

int pc_cs_rate_code(uint32_t rate)
{
    switch (rate) {
    case 32000: return 0; case 44100: return 1; case 48000: return 2; case 88200: return 3;
    case 96000: return 4; case 176400: return 5; case 192000: return 6;
    case 352800: return 7; case 384000: return 8;      /* DS 7.1.7 p.114: slave mode takes them too (the master-mode note only fixes MCLK_INT = 24.576 for 384 k) */
    default: return -1;
    }
}

bool pc_cs_rate_is_441(uint32_t rate) { return rate && rate % 11025 == 0; }

uint8_t pc_cs_vol_reg(int vol)
{
    if (vol <= 0) return 0xFF;                           /* digital mute */
    if (vol > 100) vol = 100;
    if (vol >= 10) return (uint8_t)((120 * 5 - 6 * vol + 2) / 5);          /* 120 - 1.2 * vol, rounded: 100 -> 0 (0 dB), 10 -> 108 (-54 dB) */
    return (uint8_t)(108 + ((10 - vol) * 52 + 4) / 9);   /* steeper: step 1 -> 160 (-80 dB) */
}

int pc_cs_vol_db_x10(int vol) { const uint8_t r = pc_cs_vol_reg(vol); return r == 0xFF ? -9999 : -5 * (int)r; }
float pc_cs_digital_gain(int vol) { return vol > 0 ? 1.0f : 0.0f; }

void pc_cs_set_lab(pc_cs_t *c, int hp_inv, int xtal_ibias)
{
    c->cfg_hpinv = hp_inv ? 1 : 0;
    c->cfg_ibias = (xtal_ibias == 2 || xtal_ibias == 4 || xtal_ibias == 6) ? (uint8_t)xtal_ibias : 0;   /* only the three values the checklist names */
}

void pc_cs_set_cfg(pc_cs_t *c, int hpf, int fs, int amute)
{
    c->cfg_hpf = hpf ? 1 : 0;
    c->cfg_fs = fs >= 2 ? 2 : fs == 1 ? 1 : 0;   /* 0 = 1.0 V, 1 = 1.41 V, 2 = 1.73 V (HV on) */
    c->cfg_amute = amute ? 1 : 0;
}
static uint8_t want_out_fs(const pc_cs_t *c) { return c->cfg_fs >= 2 ? PC_CS_OUT_FS_1V73 : c->cfg_fs ? PC_CS_OUT_FS_1V41 : PC_CS_OUT_FS_1V0; }
static uint8_t want_classh(const pc_cs_t *c) { return c->cfg_fs >= 2 ? PC_CS_CLASSH_HV : PC_CS_CLASSH_OFF; }   /* HV only with the 1.73 V drive (the Linux driver's mapping for loads of 600 ohm and up) */
/* the drive a register pair stands for: 0 = 1.0 V, 1 = 1.41 V, 2 = 1.73 V, 3 = any other pair (the chip is not what boot wrote) */
static int8_t drive_of(uint8_t hp, uint8_t ch)
{
    if (hp == PC_CS_OUT_FS_1V0 && ch == PC_CS_CLASSH_OFF) return 0;
    if (hp == PC_CS_OUT_FS_1V41 && ch == PC_CS_CLASSH_OFF) return 1;
    if (hp == PC_CS_OUT_FS_1V73 && ch == PC_CS_CLASSH_HV) return 2;
    return 3;
}
int pc_cs_drive_rb(const pc_cs_t *c) { return c->drive_rb; }
int pc_cs_read_drive_regs(pc_cs_t *c, uint8_t *hp, uint8_t *ch)
{
    uint8_t h = 0, k = 0;
    int e = rd(c, R_HP_OUT1, &h);
    if (!e) e = rd(c, R_CLASSH, &k);
    if (e) return e;
    *hp = h; *ch = k;
    c->drive_rb = drive_of(h, k);
    return PC_CS_OK;
}
static uint8_t pcm_base(const pc_cs_t *c) { return c->cfg_amute ? PCM_BASE : (uint8_t)(PCM_BASE & ~PCM_AMUTE); }
static uint8_t pcm_muted(const pc_cs_t *c) { return (uint8_t)(pcm_base(c) | 3); }

void pc_cs_init(pc_cs_t *c, const pc_cs_io_t *io)
{
    memset(c, 0, sizeof *c);
    c->io = *io;
    c->muted = true;
    c->cfg_hpf = 1; c->cfg_fs = 0; c->cfg_amute = 1; c->drive_rb = -1;
    c->plugged = -1;
}

int pc_cs_probe(pc_cs_t *c)
{
    c->state = PC_CS_OFF;
    c->io.reset_pin(c->io.ctx, false);      /* RESET low >= 1 ms (the rails are up: ALDO1 is on from the PMU's own start-up) */
    ms(c, 2);
    c->io.reset_pin(c->io.ctx, true);
    ms(c, 5);                                /* tPUD 1.5 ms; the address is latched in this window */
    uint8_t a = 0, b = 0, e = 0;
    int r = rd(c, R_DEVID_AB, &a);
    if (r) { ms(c, 5); r = rd(c, R_DEVID_AB, &a); }   /* one more try: the level shifters / pull-ups of a cold start */
    TRY(r);
    TRY(rd(c, R_DEVID_CD, &b));
    TRY(rd(c, R_DEVID_E, &e));
    if (a != 0x43 || b != 0x13 || (e >> 4) != 0x1) return PC_CS_EID;   /* CS43131 = 0x43131; a CS43130 reads 0x43130 and needs other sequences */
    c->state = PC_CS_PROBED;
    return PC_CS_OK;
}

int pc_cs_boot(pc_cs_t *c)
{
    if (c->state != PC_CS_PROBED) return PC_CS_ESTATE;
    uint8_t st = 0;
    TRY(rd(c, R_INT_ST, &st));                           /* clears the sticky bits (Ex. 5-7 step 5) */
    TRY(w(c, R_XTAL_IBIAS, c->cfg_ibias ? c->cfg_ibias : PC_CS_XTAL_IBIAS));           /* before the crystal starts */
    TRY(w(c, R_INT_MASK, 0xE7));                         /* only XTAL_READY / XTAL_ERROR unmasked */
    TRY(w(c, R_PDN, 0xF6));                              /* crystal on, everything else (HP amp, ASP, ...) still down */
    int ready = 0;
    for (int i = 0; i < 20 && !ready; i++) {             /* tXTAL_pup <= 6.5 ms */
        ms(c, 1);
        TRY(rd(c, R_INT_ST, &st));
        if (st & INT_XTAL_ERR) return PC_CS_EXTAL;
        ready = (st & INT_XTAL_READY) != 0;
    }
    if (!ready) return PC_CS_EXTAL;
    TRY(w(c, R_SYSCLK, 0x00));                           /* MCLK_INT = the 24.576 MHz crystal directly: no PLL, every rate */
    ms(c, 1);                                            /* >= 150 us */
    c->clk = 0;
    c->pll_bad = false;
    c->vol_ok = false;
    TRY(w(c, R_PCM_CTL1, pcm_muted(c)));                 /* muted first ... */
    c->muted = true;
    TRY(w(c, R_PCM_VOL_A, PC_CS_VOL_BOOT));              /* ... then -60 dB (the reset value) until pc_cs_set_volume */
    TRY(w(c, R_PCM_VOL_B, PC_CS_VOL_BOOT));
    TRY(w(c, R_PCM_FILTER, c->cfg_hpf ? 0x02 : 0x00));   /* fast roll-off, low latency, de-emphasis off, DC-blocking high-pass per dac_hpf (DS 7.5.2 p.128, Ex. 5-7) */
    TRY(w(c, R_CLASSH, want_classh(c)));                 /* HV off (0x1C) except at the 1.73 V drive (0x1E): earphone loads gain nothing from the 6 V rails and VP is marginal for them */
    TRY(w(c, R_HP_OUT1, want_out_fs(c)));                /* PDN_HP is still 1 here: the only time this register is written (dac_fs; 1.41 V is the ceiling with HV_EN = 0, DS 4.3 p.33; 1.73 V needs HV_EN = 1) */
    { uint8_t hp = 0, ch = 0; c->drive_rb = (rd(c, R_HP_OUT1, &hp) || rd(c, R_CLASSH, &ch)) ? -1 : drive_of(hp, ch); }   /* the honest state: what the chip says */
    TRY(w(c, R_HPDET_CTL, c->cfg_hpinv ? 0xE4 : 0xC4));  /* headphone detect on, 500 ms fall debounce (0xE4: the inverted polarity, LAB lab_dac_hp_inv) */
    TRY(w(c, R_INT_MASK, 0x9E));                         /* plug / unplug and PDN_DONE unmasked (INT is not wired: polled) */
    c->state = PC_CS_READY;
    c->rate = 0;
    c->bits = 0;
    return PC_CS_OK;
}

/* Power-down (DS Example 5-3 p.65 and 4.1.9 p.30): mute, wait 150 ms for the soft ramp, PDN_HP, a FIXED 20 ms, PDN_ASP, then read the chip id back: a part that no longer answers
 * (the datasheet's "unresponsive state" after a power-down out of order) is reported as EIO so the caller resets it.
 * PDN_DONE (INT bit0) is NOT waited for: the maker's working firmware (NightPlayer (c) 2026 ZYFDroid, MIT, cs43131.c power-down) found that it never fires while the crystal stays on
 * and uses 20 ms fixed after PDN_HP. We read it once after that time only to count a miss (done_miss, expected, not an error). The DS 5.8.1 test-register pair (0x010010 = 0x99 key,
 * 0x080046 = 0x21 before the mute, 0x20 and the key back to 0 after PDN_ASP) is for the EXTERNAL VCPFILT mode by the datasheet; NightPlayer writes it on the real board without a reported
 * pop. It is therefore an option (cfg_pdn_pair, compile switch BSP_DAC_PDN_PAIR, default OFF): off because this board runs the internal VCPFILT (CLASS_H bit0 = 0) and an undocumented
 * test register is only worth touching after an A/B listen on the board day (checklist). Step "unmute" is not done: the part stays muted and powered down. */
#define PDN_HP_WAIT_MS 20
static int stop_power_down(pc_cs_t *c)
{
    if (c->cfg_pdn_pair) { TRY(w(c, R_TEST_KEY, 0x99)); TRY(w(c, R_VCPFILT, 0x21)); }
    TRY(w(c, R_PCM_CTL1, pcm_muted(c)));                 /* the soft ramp takes the output to silence before any power-down */
    c->muted = true;
    ms(c, 150);
    uint8_t st = 0;
    TRY(rd(c, R_INT_ST, &st));                           /* clear PDN_DONE */
    TRY(rmw(c, R_PDN, 0, PDN_HP));
    ms(c, PDN_HP_WAIT_MS);
    if (rd(c, R_INT_ST, &st) != PC_CS_OK || !(st & INT_PDN_DONE)) c->done_miss++;
    TRY(rmw(c, R_PDN, 0, PDN_ASP));
    if (c->cfg_pdn_pair) { TRY(w(c, R_VCPFILT, 0x20)); TRY(w(c, R_TEST_KEY, 0x00)); }
    uint8_t id = 0;
    TRY(rd(c, R_DEVID_AB, &id));                         /* still alive? */
    return id == 0x43 ? PC_CS_OK : PC_CS_EIO;
}

void pc_cs_set_pdn_pair(pc_cs_t *c, int on) { c->cfg_pdn_pair = on ? 1 : 0; }

/* Shutdown (the PMU is about to cut the power, or a restart): the pop-free stop above while the I2S clocks still run, then NightPlayer's end of the sequence: crystal off, 2 ms,
 * RESET low and left low (hardware reset: every domain incl. the crystal off, lowest power, outputs clamped). Our order stays mute first. A failure on the way never skips the
 * RESET: the reset is the safest state. */
int pc_cs_shutdown(pc_cs_t *c)
{
    if (c->state == PC_CS_OFF) { c->io.reset_pin(c->io.ctx, false); return PC_CS_OK; }
    int rc = PC_CS_OK;
    if (c->state == PC_CS_RUNNING) rc = pc_cs_stop(c);
    else if (c->state == PC_CS_READY) w(c, R_PCM_CTL1, pcm_muted(c));
    c->muted = true;
    if (c->state != PC_CS_PROBED && c->state != PC_CS_OFF) rmw(c, R_PDN, 0, PDN_XTAL);   /* best effort */
    ms(c, 2);
    c->io.reset_pin(c->io.ctx, false);
    c->state = PC_CS_OFF; c->rate = 0; c->bits = 0; c->vol_ok = false;
    return rc;
}

int pc_cs_stop(pc_cs_t *c)
{
    if (c->state != PC_CS_RUNNING) return PC_CS_OK;
    c->want_unmute = false;                              /* the next start begins muted: the caller asks again */
    int rc = stop_power_down(c);
    if (rc) { w(c, R_PCM_CTL1, pcm_muted(c)); c->muted = true; return rc; }   /* state stays RUNNING: the caller sees the error */
    c->state = PC_CS_READY;
    c->rate = 0;
    c->bits = 0;
    return PC_CS_OK;
}

static int start_regs(pc_cs_t *c, uint32_t rate, int code, uint8_t bits);
static int vol_write(pc_cs_t *c);

int pc_cs_start(pc_cs_t *c, uint32_t rate, uint8_t bits)
{
    const int code = pc_cs_rate_code(rate);
    if (code < 0) return PC_CS_ERATE;
    if (bits != 16 && bits != 24 && bits != 32) return PC_CS_ERATE;   /* 24: BTAUDIO dac_slot24_from (ASP_SPSIZE 01, channel size 10) */
    if (c->state != PC_CS_READY && c->state != PC_CS_RUNNING) return PC_CS_ESTATE;
    if (c->state == PC_CS_RUNNING) {                      /* a rate or width change is a full stop and start (notes 5.5) */
        const int sr = pc_cs_stop(c);
        if (sr) return sr;
    }
    const int rc = start_regs(c, rate, code, bits);
    if (rc == PC_CS_OK) { c->rate = rate; c->bits = bits; return PC_CS_OK; }
    if (rc == PC_CS_ERESET) {                             /* the chip lost its registers: touching its clock or power registers could hang it (DS 4.6.1 p.44), the caller resets it */
        c->muted = true; c->state = PC_CS_PROBED; c->rate = 0; c->bits = 0; c->clk = 0;
        return rc;
    }
    /* a failure midway must not leave the amplifier up under a READY state (the next stop would skip the power-down): best effort, every write tried */
    w(c, R_PCM_CTL1, pcm_muted(c));
    c->muted = true;
    w(c, R_SYSCLK, 0x00);                                /* never power the PLL down while it is the clock: back to the crystal first */
    c->clk = 0;
    w(c, R_PDN, 0xF6);                                   /* crystal on, HP amp, ASP and PLL down (as boot leaves it) */
    w(c, R_DXD13, 0x00);
    w(c, R_TEST_KEY, 0x00);
    c->state = rc == PC_CS_ERESET ? PC_CS_PROBED : PC_CS_READY;   /* a chip that lost its registers needs the whole probe + boot again (bsp_dac) */
    c->rate = 0;
    c->bits = 0;
    return rc;
}

/* MCLK_INT for the new rate's family (DS 4.7.2.1 p.46, Table 4-4 p.45, 7.1.6 p.113): the amp and the serial port are down and the output is muted when this runs.
 * 44.1 kHz family: PLL from the 24.576 MHz crystal to 22.5792 MHz; 48 / 32 kHz family: the crystal directly. The switch always passes through the RCO (the source
 * changes only to a clock that is present), the PLL is configured, started and waited for (<= 1 ms typical) BEFORE it becomes the source; no lock = EPLL, back on the crystal. */
static int set_clock(pc_cs_t *c, int want)
{
    if (want && c->pll_bad) want = 0;                    /* the PLL did not lock before: the crystal for the rest of this boot (DS 7.1.7 p.114: any rate takes either MCLK_INT) */
    if (c->clk == want) return PC_CS_OK;
    TRY(w(c, R_SYSCLK, 0x02));                           /* MCLK_SRC_SEL = RCO */
    ms(c, 1);                                            /* >= 150 us */
    if (!want) {
        TRY(w(c, R_SYSCLK, 0x00));                       /* the crystal, MCLK_INT = 24.576 MHz */
        ms(c, 1);
        c->clk = 0;
        TRY(w(c, R_PLL_START, 0x00));
        return rmw(c, R_PDN, 0, PDN_PLL);
    }
    TRY(rmw(c, R_PDN, PDN_PLL, 0));                      /* 1. PLL block on */
    TRY(w(c, R_PLL_PREDIV, 0x03));                       /* 2. /8 */
    TRY(w(c, R_PLL_OUTDIV, 0x0A));                       /* 3. */
    TRY(w(c, R_PLL_FRAC, 0x00)); TRY(w(c, R_PLL_FRAC + 1, 0x00)); TRY(w(c, R_PLL_FRAC + 2, 0x80));   /* 4. */
    TRY(w(c, R_PLL_INT, 0x49));                          /* 5. */
    TRY(rmw(c, R_PLL_MODE, 0, 0x02));                    /* 6. PLL_MODE = 1 (bit1; the other bits keep their value: reset 0x13 in DS 7.2.8 p.117, 0x03 in the kernel) */
    TRY(w(c, R_PLL_CAL, 118));
    uint8_t st = 0;
    TRY(rd(c, R_INT_ST, &st));                           /* clear the sticky bits */
    TRY(w(c, R_INT_MASK, 0x98));                         /* 7. PLL_READY / PLL_ERROR unmasked with plug, unplug, PDN_DONE (INT is polled) */
    TRY(w(c, R_PLL_START, 0x01));                        /* 8. */
    int ok = 0;
    for (int i = 0; i < 20 && !ok; i++) {                /* typ 620 us, max 1 ms (Table 3-15) */
        ms(c, 1);
        if (rd(c, R_INT_ST, &st) != PC_CS_OK) break;
        if (st & INT_PLL_ERR) break;
        ok = (st & INT_PLL_READY) != 0;
    }
    if (ok) {
        TRY(w(c, R_SYSCLK, 0x05));                       /* MCLK_INT = 22.5792 MHz, source = PLL */
        ms(c, 1);
        c->clk = 1;
        return PC_CS_OK;
    }
    TRY(w(c, R_PLL_START, 0x00));                        /* no lock: back on the crystal and play from it (the rate's family needs 22.5792 MHz only for the filters' integer ratio), */
    TRY(w(c, R_SYSCLK, 0x00));                           /* the caller shows the reason once (note PC_CS_NOTE_PLL) */
    ms(c, 1);
    TRY(rmw(c, R_PDN, 0, PDN_PLL));
    c->clk = 0;
    c->pll_bad = true;
    c->note = PC_CS_NOTE_PLL;
    return PC_CS_OK;
}

/* the registers boot wrote (clock, output level, class H) and the crystal bit: all must still be what they were (DS 4.1.8 p.29: a POR returns every register to its default,
 * HP_OUT_CTL_1 = 0x30 at 1.73 V, CLASS_H = 0x1E with HV on, SYSCLK = RCO; the 1.73 V drive's pair IS the reset pair, so for it the SYSCLK check is what sees a POR) */
static int verify_regs(pc_cs_t *c)
{
    uint8_t pdn = 0, clk = 0, hp = 0, ch = 0;
    TRY(rd(c, R_PDN, &pdn)); TRY(rd(c, R_SYSCLK, &clk)); TRY(rd(c, R_HP_OUT1, &hp)); TRY(rd(c, R_CLASSH, &ch));
    const uint8_t want_clk = c->clk ? 0x05 : 0x00, want_fs = want_out_fs(c);
    c->drive_rb = drive_of(hp, ch);
    if ((pdn & PDN_XTAL) || clk != want_clk || hp != want_fs || ch != want_classh(c)) return PC_CS_ERESET;
    return PC_CS_OK;
}

static int start_regs(pc_cs_t *c, uint32_t rate, int code, uint8_t bits)
{
    TRY(w(c, R_PCM_CTL1, pcm_muted(c)));
    c->muted = true;
    c->want_unmute = false;
    c->vol_ok = false;
    TRY(verify_regs(c));                                 /* a reset or brown-out since the last start brought the loud defaults back: no power-up on them */
    TRY(set_clock(c, pc_cs_rate_is_441(rate) ? 1 : 0));
    TRY(w(c, R_INT_MASK, 0x9E));                         /* the status mask boot set (PLL bits masked again) */
    /* the serial port is powered down here: its format registers may be written (notes 6) */
    const uint8_t sz = bits == 32 ? 0x04 : bits == 24 ? 0x05 : 0x06;   /* ASP_SPSIZE 00 = 32 bit, 01 = 24, 10 = 16 (XSP field 01, unused; DS 7.3.10 p.120) */
    TRY(w(c, R_SPRATE, (uint8_t)code));
    TRY(w(c, R_SPSIZE, sz));
    TRY(w(c, R_LCHI, (uint8_t)(bits - 1))); TRY(w(c, R_LCHI + 1, 0));
    TRY(w(c, R_LCPR, (uint8_t)(2 * bits - 1))); TRY(w(c, R_LCPR + 1, 0));
    TRY(w(c, R_ASP_CLK, 0x0C));                          /* slave, SCLK / LRCK polarities of Philips I2S */
    TRY(w(c, R_ASP_FRAME, 0x0A));                        /* 50/50 frame, 1.0 SCLK data delay (I2S) */
    TRY(w(c, R_CH1_LOC, 0x00)); TRY(w(c, R_CH2_LOC, 0x00));
    TRY(w(c, R_CH1_CFG, bits == 32 ? 0x07 : bits == 24 ? 0x06 : 0x05));      /* left: LRCK low, enabled, RES 11 = 32 / 10 = 24 / 01 = 16 bit (DS 7.3.23 p.121) */
    TRY(w(c, R_CH2_CFG, bits == 32 ? 0x0F : bits == 24 ? 0x0E : 0x0D));      /* right: LRCK high (DS 7.3.24) */
    TRY(vol_write(c));                                   /* the volume, read back, before anything can be unmuted */
    /* pop-free power-up (DS Ex. 5-5, CS43131 / CS43198 only) */
    TRY(w(c, R_TEST_KEY, 0x99));
    TRY(w(c, R_DXD13, 0x20));
    TRY(rmw(c, R_PDN, PDN_ASP, 0));
    TRY(rmw(c, R_PDN, PDN_HP, 0));
    ms(c, 12);
    TRY(w(c, R_DXD13, 0x00));
    TRY(w(c, R_TEST_KEY, 0x00));
    c->state = PC_CS_RUNNING;                            /* still muted: the caller unmutes */
    return PC_CS_OK;
}

/* PCM_VOLUME_A/B for c->vol, then both read back: vol_ok only when they match */
static int vol_write(pc_cs_t *c)
{
    const uint8_t r = pc_cs_vol_reg(c->vol);
    uint8_t a = 0;
    c->vol_ok = false;
    TRY(w(c, R_PCM_VOL_A, r));                           /* PCM_VOL_BEQA is set (pcm_base): channel B follows A in the same instant */
    TRY(rd(c, R_PCM_VOL_A, &a));
    if (a != r) return PC_CS_EVOL;
    c->vol_ok = true;
    return PC_CS_OK;
}

/* unmute only when the caller wants sound, the part runs, the volume is above 0 and was read back */
static int unmute_if_due(pc_cs_t *c)
{
    if (c->state != PC_CS_RUNNING || !c->want_unmute || c->vol <= 0 || !c->vol_ok || !c->muted) return PC_CS_OK;
    TRY(w(c, R_PCM_CTL1, pcm_base(c)));
    c->muted = false;
    return PC_CS_OK;
}

int pc_cs_set_volume(pc_cs_t *c, int vol)
{
    c->vol = vol < 0 ? 0 : vol > 100 ? 100 : vol;
    if (c->state < PC_CS_READY) return PC_CS_OK;         /* the value waits: boot / start write it */
    if (c->state == PC_CS_RUNNING && c->vol == 0 && !c->muted) {   /* to silence: mute first, then the register */
        TRY(w(c, R_PCM_CTL1, pcm_muted(c)));
        c->muted = true;
    }
    int rc = vol_write(c);
    if (rc) {                                            /* a register that does not read back: never sound */
        if (c->state == PC_CS_RUNNING && !c->muted) { w(c, R_PCM_CTL1, pcm_muted(c)); c->muted = true; }
        return rc;
    }
    return unmute_if_due(c);
}

int pc_cs_mute(pc_cs_t *c, bool mute)
{
    if (!mute && c->state != PC_CS_RUNNING) return PC_CS_ESTATE;   /* never unmute a part that is not running (or not configured) */
    c->want_unmute = !mute;
    if (c->state == PC_CS_OFF || c->state == PC_CS_PROBED) return PC_CS_OK;
    if (mute) {
        TRY(w(c, R_PCM_CTL1, pcm_muted(c)));
        c->muted = true;
        return PC_CS_OK;
    }
    return unmute_if_due(c);                             /* volume 0 or unverified: stays muted (OK), the volume write unmutes later */
}

int pc_cs_serial_errors(pc_cs_t *c)
{
    if (c->state != PC_CS_RUNNING) return 0;
    uint8_t v = 0;
    if (rd(c, R_INT_ST2, &v)) return -1;
    return v & 0x78;                                     /* NOLRCK, EARLY, LATE, ERROR (sticky: this read clears them) */
}

int pc_cs_headphone(pc_cs_t *c)
{
    if (c->state < PC_CS_READY) return -1;
    uint8_t v = 0;
    if (rd(c, R_HPDET_ST, &v)) return -1;
    if (v & 0x40) c->plugged = 1;
    else if (v & 0x20) c->plugged = 0;
    return c->plugged;
}
#endif  // BOARD_FAKEPOD_NANO_PRO
