// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
/* FakePod (ESP32-S31 + CO5300 AMOLED) pin map. Source: the maker's hardware guide GPIO table + schematic FAKE_POD_NANO_S31 V1.0 (read 2026-11-12, .local/fakepod/hw-notes.md).
 * Do not use bare GPIO numbers anywhere else in the code.
 * HAZARDS: GPIO36 (LCD reset) is also the flash-voltage strap (10k pull-up = 3.3 V): it is only ever driven high, or pulsed low for the panel reset while running, never left low.
 *          GPIO61 (bottom key) and GPIO60 are boot straps: input only, no internal pull. The AXP2101 powers everything: this firmware reads its key IRQ and writes only the IRQ
 *          mask / status (bsp_keys.c), never a voltage, charger or rail register (ALDO1 must stay 1.8 V, DCDC1 on). F2 adds READS only (bsp_pmu.c: status, battery ADC, gauge). */
#include "driver/gpio.h"
#include "driver/i2c_types.h"
#include "driver/i2s_types.h"
#include "driver/spi_common.h"
#include "soc/soc_caps.h"
#include "bsp/bsp_geom.h"

/* Display: CO5300 460x460 over QSPI; AMOLED: no backlight pin, the brightness is a panel command */
#define BSP_LCD_SPI_HOST         SPI2_HOST
#define BSP_PIN_LCD_SCL          GPIO_NUM_38
#define BSP_PIN_LCD_CS           GPIO_NUM_37
#define BSP_PIN_LCD_D0           GPIO_NUM_39
#define BSP_PIN_LCD_D1           GPIO_NUM_40
#define BSP_PIN_LCD_D2           GPIO_NUM_42
#define BSP_PIN_LCD_D3           GPIO_NUM_43
#define BSP_PIN_LCD_RST          GPIO_NUM_36   /* ALSO the flash-voltage strap: see above */
#define BSP_PIN_LCD_TE           GPIO_NUM_35   /* not used (no tear sync) */
#define BSP_PIN_LCD_VCI_EN       GPIO_NUM_44   /* AMOLED supply enable, high = on (10k pull-down on the FPC) */
#define BSP_LCD_X_GAP            10            /* the panel's RAM columns start at 10 on every window */

/* I2C0: touch CST820 + AXP2101 PMU + QMI8658 IMU (+ the RTC), 2.2k pull-ups on the board */
#define BSP_PIN_TP_SDA           GPIO_NUM_48
#define BSP_PIN_TP_SCL           GPIO_NUM_47
#define BSP_PIN_TP_INT           GPIO_NUM_45
#define BSP_PIN_TP_RST           GPIO_NUM_46
#define BSP_I2C_PORT_TOUCH       I2C_NUM_0
#define BSP_I2C_FREQ_HZ          400000
#define BSP_I2C_ADDR_TOUCH       0x15
#define BSP_I2C_ADDR_PMU         0x34          /* AXP2101 */
#define BSP_I2C_ADDR_IMU_PRIMARY 0x6B          /* the guide says 0x6B, the schematic's SDO to GND says 0x6A: both are probed */
#define BSP_I2C_ADDR_IMU_ALT     0x6A
#define BSP_PIN_PMU_IRQ          GPIO_NUM_49   /* AXP2101 nIRQ through a level shifter, low = an event */

/* Keys: the top one is the PMU's power key (no GPIO: its edges come as AXP2101 IRQs; its long hold is the hardware power-off), the bottom one is BOOTMODE */
#define BSP_PIN_KEY_BOTTOM       GPIO_NUM_61   /* 10k pull-up on the board, pressed = low */
#define BSP_PIN_BOOT             BSP_PIN_KEY_BOTTOM

/* PDM microphone MSM261D4030 */
#define BSP_PIN_MIC_CLK          GPIO_NUM_6
#define BSP_PIN_MIC_DATA         GPIO_NUM_2

/* TF card, SDMMC 4-bit (the same GPIOs as the round board) */
#define BSP_PIN_SD_CLK           GPIO_NUM_24
#define BSP_PIN_SD_CMD           GPIO_NUM_25
#define BSP_PIN_SD_D0            GPIO_NUM_20
#define BSP_PIN_SD_D1            GPIO_NUM_21
#define BSP_PIN_SD_D2            GPIO_NUM_22
#define BSP_PIN_SD_D3            GPIO_NUM_23

/* CS43131 DAC (F2, pc_cs43131 + bsp_dac.c; written blind): I2S slave to the SoC master, its own I2C1 behind 1.8 V level shifters. BSP_DAC_CS43131 makes the shared bsp_audio.c
 * drive it instead of the round board's XSMT pin. */
#ifndef BSP_DAC_CS43131          /* also a PUBLIC compile definition of this component (CMakeLists): every file of the FakePod build sees it, with or without this header */
#define BSP_DAC_CS43131          1
#endif
#define BSP_I2C_PORT_DAC         I2C_NUM_1
#define BSP_I2C_FREQ_DAC_HZ      100000        /* through a TXS0102 with 2.2 k pull-ups: slow first (the part does 400 k) */
#define BSP_PIN_I2S_BCK          GPIO_NUM_52
#define BSP_PIN_I2S_WS           GPIO_NUM_51
#define BSP_PIN_I2S_DOUT         GPIO_NUM_50
#define BSP_PIN_DAC_RST          GPIO_NUM_53
#define BSP_PIN_DAC_SDA          GPIO_NUM_55
#define BSP_PIN_DAC_SCL          GPIO_NUM_54
#define BSP_I2C_ADDR_DAC         0x30

#define BSP_I2S_PORT_MIC         I2S_NUM_0
#define BSP_I2S_PORT_DAC         I2S_NUM_1
/* No haptic driver, buzzer, knob, LED, backlight pin or battery ADC on this board (no BSP_PIN_LCD_BLK / BSP_PIN_DAC_XSMT / BSP_PIN_BUZZER / BSP_PIN_BATT_ADC). */
