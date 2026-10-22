# Nano35 Charybdis：dongle 模式

`charybdis-Nano35` 使用三個 `nice_nano_v2`：

| 韌體 shield | 用途 |
| --- | --- |
| `charybdis_dongle` | 中央端，透過 BLE 連接左右半部，透過 USB 輸出鍵盤及滑鼠，提供 Studio USB 連線 |
| `charybdis_left` | 左半部周邊端 |
| `charybdis_right` | 右半部周邊端，傳送按鍵、旋鈕與 PMW3610 軌跡球資料 |
| `settings_reset` | 清除控制器的配對與儲存設定 |

按鍵配置及 Nano35 的 35 鍵實體配置沿用 `config/charybdis.keymap` 與現有 shield。
第 1 層的軌跡球捲動改由 dongle 處理，比例為 70 單位／格；其他層維持游標移動。
RGB 保留在左右半部，dongle 使用沒有實體輸出的 RGB 狀態裝置，讓全域 RGB 控制仍可轉送至兩側。
PMW3610 驅動固定版本；`config/charybdis.c` 為舊版驅動提供周邊端相容處理，分層邏輯只在 dongle 執行。

GitHub Actions 會依 `build.yaml` 建置以上四種韌體。Studio 的 `studio-rpc-usb-uart` snippet 只套用在 dongle。
下載方式：登入 GitHub，開啟本專案的 **Actions → Build Nano35 dongle firmware**，選擇 `charybdis-Nano35` 分支最近一次成功的執行，在 **Artifacts** 下載 `firmware` ZIP。
其中包含左右半部、dongle 與設定重置的四個 UF2，檔名格式為 `<shield>-nice_nano_v2-zmk.uf2`。

在已設定好的雲端環境可執行：

```bash
bash /workspace/.zmk-env/build.sh all
```

輸出位於 `/workspace/.zmk-env/firmware/`，工具、建置檔及記錄均留在專案外。

首次從原本的兩半模式改為 dongle 模式時：

1. 左半部、右半部、dongle 三個控制器都先刷入 `settings_reset`，等待韌體啟動並清除設定。這也會清除已儲存的 Studio 改鍵。
2. 再分別刷入 `charybdis_left`、`charybdis_right`、`charybdis_dongle` 對應的 UF2。
3. 將 dongle 接上電腦 USB，讓左右半部開機並靠近 dongle，自動建立 BLE 配對。Studio 也改連線至 dongle。

左右半部的 USB 保留刷機用途；日常按鍵及軌跡球輸出由 dongle 負責。
雲端建置可確認韌體可編譯；BLE 配對、RGB、旋鈕及軌跡球仍須在實體鍵盤確認。
