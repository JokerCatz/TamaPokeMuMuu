// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
/* The FakePod's CS43131 as the shared bsp_audio.c uses it (BSP_DAC_CS43131): the SoC's I2S master feeds it, this controls it over its own I2C1 (GPIO55 / 54) with the
 * RESET line on GPIO53. All logic is pc_cs43131 (pure, host-tested); this file is the transport, a mutex and the log lines. Not run on hardware (F2, written blind).
 * DACFS (owner 2026-11-16): the DAC's own digital volume is the volume of the wired path (bsp_dac_set_volume); the clock family and the rate are reprogrammed per track. Any task. */
esp_err_t bsp_dac_init(void);                       /* reset, chip id, crystal, fixed level, muted; idempotent, retried after a failure */
esp_err_t bsp_dac_start(uint32_t rate, uint8_t bits);   /* the I2S clocks must run: serial port + amp up, still muted. ESP_ERR_NOT_SUPPORTED: no ASP_SPRATE code (below 32 kHz); ESP_FAIL: PLL no lock / volume not read back (stays muted) */
bool      bsp_dac_cfg_pending(void);                   /* FIX4 I1: dac_hpf / dac_fs / dac_amute changed and not yet applied to the chip */
void      bsp_dac_config(int hpf, int fs, int amute);   /* dac_hpf / dac_fs / dac_amute: before bsp_dac_init (boot-time settings) */
esp_err_t bsp_dac_set_volume(int vol);               /* 0..100 -> PCM_VOLUME_A/B (pc_cs_vol_reg), 0 = mute; stored until init; unmutes only after a read-back */
esp_err_t bsp_dac_stop(void);
int       bsp_dac_serial_errors(void);              /* RUNNING: the ASP error bits (NOLRCK / EARLY / LATE / ERROR) since the last call, 0 none or unknown */
#define BSP_DAC_NOTE_PLL  1                         /* the PLL did not lock: the rate plays from the crystal clock */
#define BSP_DAC_NOTE_ABSENT 4                       /* no CS43131 answered at all at boot (a PCM5102 variant of the board has no I2C DAC): the wired output stays refused, said once */
#define BSP_DAC_NOTE_DEAD 2                         /* the DAC stopped answering / lost its registers: RESET low, muted until it is initialised again */
uint8_t   bsp_dac_take_note(void);                  /* a thing to tell the user once (BSP_DAC_NOTE_*), 0 none; clears it */
bool      bsp_dac_fault(void);                      /* faulted and not yet re-initialised */                       /* mute, ramp, amp and serial port down (call before the I2S clocks stop) */
esp_err_t bsp_dac_mute(bool mute);                  /* unmute only while running; muting always */
int       bsp_dac_headphone(void);                  /* 1 plugged, 0 out, -1 unknown (the detect's polarity is a bring-up item) */
bool      bsp_dac_absent(void);                     /* the probe found no CS43131 (no answer or another chip id) and no earlier init ever succeeded: wired output is refused */
void      bsp_dac_shutdown(void);                   /* PMU power-off / restart: pop-free stop, crystal off, RESET low and held (pc_cs_shutdown); call before the I2S clocks stop */
#ifndef BSP_DAC_PDN_PAIR
#define BSP_DAC_PDN_PAIR 0                          /* board-day option (checklist): 1 = the datasheet's 0x080046 test-register pair around the power-down, as NightPlayer does; default off (internal VCPFILT) */
#endif
int       bsp_dac_drive(void);                      /* the output drive the chip's registers showed at boot / the last start: -1 unknown, 0 1.0 V, 1 1.41 V, 2 1.73 V, 3 an unexpected pair */
int       bsp_dac_drive_set(void);                  /* the drive the chip was last configured for (0 / 1 / 2) */
void      bsp_dac_dump(void);                       /* console `dac` */
