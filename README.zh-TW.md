# TamaPokeMuMuu（繁體中文）

[English](README.md) · [简体中文](README.zh-CN.md) · **繁體中文**

這是 [Mu-Muu/TamaPoke](https://github.com/Mu-Muu/TamaPoke)（TamaPoke with Chess，MIT）的分支，讓這隻寶可夢電子雞也能在方形 460×460 的 **Fake Pod Nano Pro（ESP32-S31）** 上執行。

## 新增內容
- Fake Pod 硬體支援：CO5300 AMOLED、CST820 觸控、AXP2101、PCF85063 RTC、QMI8658、CS43131 耳機 DAC 音效、側鍵。
- TF 卡可用 FAT32 或 exFAT（透過 SdFat），韌體絕不會格式化卡片。
- **繁體中文（台灣）**：介面、1025 隻寶可夢官方繁體名稱、道館與徽章、Wi-Fi 設定網頁。
- 第一次開機先選語言（English / 繁體中文 / 简体中文）。
- 方形螢幕：背景填滿整個畫面，沒有圓形遮罩；中文最小字體 24 px。

## 安裝 release（Fake Pod Nano Pro）
1. 從[最新的 Fake Pod release](https://github.com/JokerCatz/TamaPokeMuMuu/releases) 下載 `TamaPokeMuMuu-fakepod-0.1.0-merged.bin` 與 `TamaPokeMuMuu-sdcard-mons.zip`。
2. **TF 卡**（FAT32 或 exFAT，韌體不會格式化）：把 zip 解壓縮到卡上，讓圖片放在 `/mons/`。
3. **韌體**：讓板子進入下載模式（插 USB 時同時按住兩個側鍵），然後執行
   `esptool --chip esp32s31 --port <連接埠> write_flash 0x0 TamaPokeMuMuu-fakepod-0.1.0-merged.bin`
   release 內建的時區是 UTC+8；其他時區請自行從原始碼建置（見下）。
4. 第一次開機會先選語言。想從頭開始，清除 NVS 區：`esptool --chip esp32s31 --port <連接埠> erase_region 0x9000 0x5000`。

## 建置與刷機
請看 [tools/fakepod/README.md](tools/fakepod/README.md)（英文）。大致步驟：安裝 arduino-esp32 4.0.0 與函式庫、執行 `tools/fakepod/patch_arduino_gfx.sh`、複製 `config_private.example.h` 為 `config_private.h` 並設定時區，然後執行 `tools/build_fakepod.sh --upload 連接埠`。

## 圖片包
寶可夢圖片（`/mons/`）放在 TF 卡上，來源是上游專案；可用 `tools/fakepod/sd_uploader` 與 `tools/send_sd_fp.py` 透過 USB 傳到卡上。

## 授權
MIT，沿用上游。詳見 [LICENSE](LICENSE) 與 [CREDITS.md](CREDITS.md)。
