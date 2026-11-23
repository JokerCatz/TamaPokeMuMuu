// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
/* Display geometry of the FakePod (ESP32-S31 "Fake Pod Nano Pro"): 460x460 AMOLED, rectangular, no knob (two keys stand in), a QMI8658 IMU. No IDF headers, so host code and
 * the simulator can include it (the round board's file has the same names). */
#define BSP_LCD_H_RES            460
#define BSP_LCD_V_RES            460
#define BSP_LCD_ROUND            0      /* rectangular glass */
#define BSP_HAS_KNOB             0      /* the two keys are the knob (keys_svc: ui_knob_inject) */
#define BSP_HAS_KEYS             1
#define BSP_HAS_IMU              1      /* QMI8658: the maze's tilt rule (imu_svc) */
#define BSP_HAS_PMU              1      /* AXP2101 reads: battery %, charging, VBUS (bsp_pmu.h, power_svc) */
#define BSP_PANEL_OLED           1      /* AMOLED: a lit picture burns in (the idle screen timeout always applies, ui.c / pc_pwrpol.h) */
#define BSP_BOARD_ID             "fakepod"
/* How the QMI8658's axes lie against the screen's (x right, y down). Not in the schematic: the bring-up checklist's "IMU axes" step finds them (console `imu map`),
 * then these constants are set. See pc_imu.h. */
#define BSP_IMU_SWAP_XY          1
#define BSP_IMU_SIGN_X           -1
#define BSP_IMU_SIGN_Y           -1
