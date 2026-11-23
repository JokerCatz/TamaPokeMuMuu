#pragma once

// Pines oficiales de la Waveshare ESP32-S3-Touch-AMOLED-1.75
// Fuente: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75 (libraries/Mylibrary/pin_config.h)

#if defined(BOARD_FAKEPOD_NANO_PRO)
// Fake Pod Nano Pro (ESP32-S31): 460x460 CO5300 AMOLED over QSPI, CST820 touch, AXP2101, PCF85063, QMI8658, CS43131 headphone DAC (no speaker), TF slot.
// Pin map from the JukeDial / TamaPoke-FakePod projects (same board, verified there). GPIO36 (LCD reset) is also the flash-voltage strap: only pulsed low while running.
#define XPOWERS_CHIP_AXP2101
#define LCD_SDIO0 39
#define LCD_SDIO1 40
#define LCD_SDIO2 42
#define LCD_SDIO3 43
#define LCD_SCLK 38
#define LCD_CS 37
#define LCD_RESET 36
#define LCD_VCI_EN 44          // AMOLED supply enable (high = on). NOT an AXP2101 rail: no PMU voltage register is written on this board
#define LCD_WIDTH 460
#define LCD_HEIGHT 460
#define LCD_COL_OFFSET 10      // the panel's RAM columns start at 10
#define DISPLAY_BEGIN_HZ 40000000  // 80 MHz draws a stray black line on partial redraws (the maker's finding)
#define IIC_SDA 48
#define IIC_SCL 47
#define TP_INT 45
#define TP_RESET 46
#define TOUCH_ADDR 0x15        // CST820
#define TOUCH_DRIVER TouchDrvCST816
#define TOUCH_X_NUM 29         // the maker's x calibration for this panel: x * 29 / 30
#define TOUCH_X_DEN 30
#define TOUCH_MIRROR_X false
#define TOUCH_MIRROR_Y false
// TF slot, SDMMC 4-bit through SdFat (sdfat_fs.cpp): exFAT cards work (arduino-esp32's own FatFs has no exFAT)
#define SDMMC_CLK 24
#define SDMMC_CMD 25
#define SDMMC_DATA 20
#define SDMMC_D1 21
#define SDMMC_D2 22
#define SDMMC_D3 23
// side keys: the top one is the AXP2101 power key (nIRQ on GPIO49), the bottom one is BOOTMODE (GPIO61, a strapping pin: input only, never hold it at reset)
#define PMU_IRQ_PIN 49
#define KEY_BOTTOM_PIN 61
// CS43131 headphone DAC (audio_fakepod.cpp)
#define I2S_BCK_IO 52
#define I2S_WS_IO 51
#define I2S_DO_IO 50
#else
#define XPOWERS_CHIP_AXP2101
// Pantalla AMOLED 466x466, driver CO5300 por QSPI
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 39
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Táctil capacitivo CST9217 por I2C
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 40

// Audio ES8311. NOTA: el MCLK real es GPIO42 (verificado en placa con el
// proyecto PlaneRadar2.0); el 16 que figuraba era erroneo. Aun asi el codec se
// configura con reloj derivado del BCLK, asi que el MCLK apenas importa.
#define I2S_MCK_IO 42
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8
#define PA 46

// Ranura TF (no usada todavía)
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 3
#define SDMMC_CS 41
#define LCD_COL_OFFSET 6
#define DISPLAY_BEGIN_HZ 80000000
#define TOUCH_ADDR 0x5A        // CST9217
#define TOUCH_DRIVER TouchDrvCST92xx
#define TOUCH_X_NUM 1
#define TOUCH_X_DEN 1
#define TOUCH_MIRROR_X true    // the panel is mounted rotated 180 degrees
#define TOUCH_MIRROR_Y true
#endif
