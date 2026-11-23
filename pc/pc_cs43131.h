// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* CS43131 DAC driver logic (FakePod, F2), pure: every register access goes through the io table, so the host test drives it against a model of the chip and checks the
 * ORDER of the writes (the pop-free sequence, the mute before every power-down) and the output level. Written WITHOUT the board from the datasheet DS1155F2 and the
 * notes in .local/fakepod/cs43131-notes.md (section numbers in the comments): nothing here has run on hardware; the checklist's FakePod section lists the steps.
 * Wiring it assumes: the SoC is the I2S master (BCK/LRCK/SDIN, 32-bit or 16-bit slots, 64 / 32 BCK per frame), the DAC a slave on its own 24.576 MHz crystal (no MCLK
 * from the SoC), control over I2C at 0x30 with 24-bit register addresses.
 *
 * AUDIO SAFETY (DACFS, owner ruling 2026-11-16, FakePod only): the DAC's own digital volume (PCM_VOLUME_A/B, 0.5 dB steps, 0 dB .. -127.5 dB, 0xFF = digital mute) is
 * THE volume of the wired path; audio_svc passes unity on the FakePod and sends the volume here (pc_cs_set_volume). The chip starts muted (boot), is muted before
 * every reconfiguration (rate change, PLL, power-down) and unmuted only by pc_cs_mute(false) / a volume rise AFTER the volume registers were written and read
 * back equal in this run (vol_ok), never at volume 0 (the mute bits stay set there). The full-scale output voltage is OUT_FS (1.0 V default; 1.41 V and, experimental, 1.73 V with Class H HV_EN on, by the
 * dac_fs setting 0 / 1 / 2; written only while the amp is down). */
#define PC_CS_VOL_BOOT      0x78   /* PCM_VOLUME_A/B at boot: -60 dB (the chip's reset value), under the mute */
#define PC_CS_OUT_FS_1V0    0x10   /* HP Output Control 1: OUT_FS = 01 (1.0 V rms full scale), +1 dB off. Only written while the headphone amp is powered down */
#define PC_CS_OUT_FS_1V41   0x20   /* OUT_FS = 10 (1.41 V rms): the highest full scale with HV_EN = 0 (DS 4.3 p.33); 1.73 V needs HV on and loads of 600 ohm and up */
#define PC_CS_OUT_FS_1V73   0x30   /* OUT_FS = 11 (1.73 V rms, +4.8 dB over 1.0 V): only with PC_CS_CLASSH_HV; the datasheet applies HV to loads of 600 ohm and up and needs VP >= 3.3 V (the board's supply is at that minimum) */
#define PC_CS_CLASSH_OFF    0x1C   /* Class H Control: HV_EN = 0 (+/-2.6 V rails): the 1.0 V and 1.41 V drives */
#define PC_CS_CLASSH_HV     0x1E   /* HV_EN = 1 (+/-3.0 V rails; the chip's reset value): the 1.73 V drive only */
#define PC_CS_XTAL_IBIAS    0x04   /* XTAL_IBIAS (12.5 uA, the reset value): try 0x06 then 0x02 if the crystal does not start (notes 5.1) */

enum { PC_CS_OK = 0, PC_CS_EIO = -1, PC_CS_EID = -2, PC_CS_EXTAL = -3, PC_CS_ERATE = -4, PC_CS_ESTATE = -5, PC_CS_EPLL = -7, PC_CS_EVOL = -8, PC_CS_ERESET = -9 };   /* EPLL is no longer returned: no lock = the crystal + note PC_CS_NOTE_PLL; ERESET: the chip lost its registers (state PROBED) */
enum { PC_CS_NOTE_NONE = 0, PC_CS_NOTE_PLL = 1 };
enum { PC_CS_OFF = 0, PC_CS_PROBED, PC_CS_READY, PC_CS_RUNNING };   /* READY: crystal on, headphone amp and serial port powered down (standby) */

typedef struct {
    void *ctx;
    int  (*wr)(void *ctx, uint32_t reg, uint8_t v);        /* 0 = ok */
    int  (*rd)(void *ctx, uint32_t reg, uint8_t *v);       /* 0 = ok */
    void (*delay_ms)(void *ctx, uint32_t ms);
    void (*reset_pin)(void *ctx, bool high);               /* the RESET# line (GPIO53 through the level shifter) */
} pc_cs_io_t;

typedef struct {
    pc_cs_io_t io;
    uint8_t state;
    uint8_t bits;      /* slot width of the running stream */
    uint32_t rate;
    bool muted;        /* the DAC's PCM mute bits (true from boot and through every stop) */
    int8_t plugged;    /* last headphone-detect verdict: -1 unknown, 0 out, 1 in */
    uint8_t cfg_pdn_pair; /* board-day option (default 0): the 0x080046 test-register pair around the power-down, see stop_power_down */
    uint8_t cfg_hpinv, cfg_ibias;         /* LAB (pc_lab.h): headphone-detect polarity 0 / 1 (0 = as built), XTAL_IBIAS 2 / 4 / 6 (0 = PC_CS_XTAL_IBIAS); pc_cs_set_lab, before boot */
    uint8_t cfg_hpf, cfg_fs, cfg_amute;   /* settings dac_hpf (1), dac_fs (0 = 1.0 V, 1 = 1.41 V, 2 = 1.73 V), dac_amute (1): set before boot (pc_cs_set_cfg) */
    int8_t drive_rb;   /* the drive the chip's own registers show after boot / the last start: -1 unknown, 0 = 1.0 V, 1 = 1.41 V, 2 = 1.73 V, 3 = a pair boot did not write */
    uint8_t clk;       /* MCLK_INT: 0 = the 24.576 MHz crystal directly (48 kHz family), 1 = the PLL's 22.5792 MHz (44.1 kHz family) */
    int vol;           /* last volume 0..100 asked for (0 = silent) */
    bool vol_ok;       /* the volume registers were written and read back equal in this run */
    uint16_t done_miss; /* stops whose PDN_DONE never showed in the bounded wait (expected with the crystal on; not an error) */
    bool pll_bad;      /* the PLL did not lock once: the crystal is used for every rate from then on (this boot) */
    uint8_t note;      /* PC_CS_NOTE_*: something the user should be told once (bsp_dac takes and clears it) */
    bool want_unmute;  /* the caller asked for sound: honoured as soon as the volume is above 0 and verified */
} pc_cs_t;

int  pc_cs_rate_code(uint32_t rate);                       /* ASP_SPRATE code (DS 7.1.7 p.114) for 32 / 44.1 / 48 / 88.2 / 96 / 176.4 / 192 / 352.8 / 384 kHz, -1 for anything else (below 32 kHz the part has no code) */
bool pc_cs_rate_is_441(uint32_t rate);                     /* the 44.1 kHz family (MCLK_INT 22.5792 MHz through the PLL); the 32 / 48 kHz family runs on the crystal's 24.576 MHz */
uint8_t pc_cs_vol_reg(int vol);                            /* volume 1..100 -> PCM_VOLUME code: 100 = 0 dB (0x00), 0.6 dB per step down to step 10 (-54 dB), steeper below (step 1 = -80 dB); 0 -> 0xFF digital mute */
int  pc_cs_vol_db_x10(int vol);                            /* the DAC's level for a volume in 0.1 dB (0 = 0 dB, -5 per register step); -9999 for the digital mute (volume 0) */
float pc_cs_digital_gain(int vol);                         /* the digital gain stage on the FakePod: 1.0 (unity) for vol > 0, 0 for 0; the DAC's register does the volume */
void pc_cs_set_lab(pc_cs_t *c, int hp_inv, int xtal_ibias);   /* LAB: before boot; hp_inv 0 / 1, xtal_ibias 2 / 4 / 6 (anything else = the default) */
void pc_cs_set_cfg(pc_cs_t *c, int hpf, int fs, int amute);   /* before boot: dac_hpf, dac_fs, dac_amute */
int  pc_cs_drive_rb(const pc_cs_t *c);                      /* -1 unknown, 0 / 1 / 2 the drive read back from HP_OUT_CTL_1 + CLASS_H (1.0 / 1.41 / 1.73 V), 3 an unexpected pair */
int  pc_cs_read_drive_regs(pc_cs_t *c, uint8_t *hp, uint8_t *ch);   /* a live read of HP_OUT_CTL_1 and CLASS_H (also refreshes drive_rb); 0 ok, else the bus error */
int  pc_cs_set_volume(pc_cs_t *c, int vol);                /* write + read back PCM_VOLUME_A/B (vol_ok); 0 mutes first; a rise from 0 unmutes only after the read-back when the caller wants sound */
void pc_cs_init(pc_cs_t *c, const pc_cs_io_t *io);
int  pc_cs_probe(pc_cs_t *c);                              /* reset pulse, then the chip id (0x43 0x13 0x1x) must read back: PC_CS_EID otherwise */
int  pc_cs_boot(pc_cs_t *c);                               /* crystal, mute, fixed level, class H, headphone detect: ends READY (standby) and muted */
int  pc_cs_start(pc_cs_t *c, uint32_t rate, uint8_t bits); /* the SoC's I2S clocks must already run; programs the clock family (PLL), serial port, pop-free power-up; ends RUNNING and muted. PC_CS_EPLL: no lock (READY, muted) */
void pc_cs_set_pdn_pair(pc_cs_t *c, int on);                 /* before boot: the DS 5.8.1 0x080046 pair in the power-down (default off; owner A/B listen on the board day) */
int  pc_cs_shutdown(pc_cs_t *c);                           /* stop (if running), crystal off, 2 ms, RESET low and held: state OFF, muted. For the PMU power-off; always ends with RESET low */
int  pc_cs_stop(pc_cs_t *c);                               /* mute, wait the soft ramp, headphone amp down (a fixed 20 ms, PDN_DONE is not waited for), serial port down: READY */
int  pc_cs_mute(pc_cs_t *c, bool mute);                    /* PCM mute bits; unmuting is refused (PC_CS_ESTATE) unless RUNNING, and stays muted (OK) while vol is 0 or not verified */
int  pc_cs_serial_errors(pc_cs_t *c);                      /* RUNNING: the ASP error bits (0x78 mask: NOLRCK / EARLY / LATE / ERROR) since the last read, -1 on a failed read, 0 otherwise */
int  pc_cs_headphone(pc_cs_t *c);                          /* 1 plugged, 0 out, -1 not decided yet (the chip's debounce is running) or a read failed */
#ifdef __cplusplus
}
#endif
