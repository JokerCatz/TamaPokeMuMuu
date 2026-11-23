# TamaPokeMuMuu（繁體中文）

[English](README.md) · [简体中文](README.zh-CN.md) · **繁體中文**

這是 [Mu-Muu/TamaPoke](https://github.com/Mu-Muu/TamaPoke)（TamaPoke with Chess，MIT）的分支，讓這隻寶可夢電子雞也能在方形 460×460 的 **Fake Pod Nano Pro（ESP32-S31）** 上執行。

## 新增內容
- Fake Pod 硬體支援：CO5300 AMOLED、CST820 觸控、AXP2101、PCF85063 RTC、QMI8658、CS43131 耳機 DAC 音效、側鍵。
- TF 卡可用 FAT32 或 exFAT（透過 SdFat），韌體絕不會格式化卡片。
- **繁體中文（台灣）**：介面、1025 隻寶可夢官方繁體名稱、道館與徽章、Wi-Fi 設定網頁。
- 第一次開機先選語言（English / 繁體中文 / 简体中文）。
- 方形螢幕：背景填滿整個畫面，沒有圓形遮罩；中文最小字體 24 px。

## 建置與刷機
請看 [tools/fakepod/README.md](tools/fakepod/README.md)（英文）。大致步驟：安裝 arduino-esp32 4.0.0 與函式庫、執行 `tools/fakepod/patch_arduino_gfx.sh`、複製 `config_private.example.h` 為 `config_private.h` 並設定時區，然後執行 `tools/build_fakepod.sh --upload 連接埠`。

## 圖片包
寶可夢圖片（`/mons/`）放在 TF 卡上，來源是上游專案；可用 `tools/fakepod/sd_uploader` 與 `tools/send_sd_fp.py` 透過 USB 傳到卡上。

## 授權
MIT，沿用上游。詳見 [LICENSE](LICENSE) 與 [CREDITS.md](CREDITS.md)。
