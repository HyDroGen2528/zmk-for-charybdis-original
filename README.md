# Nano35 Charybdis：dongle 模式

`charybdis-Nano35` 使用三個 `nice_nano_v2`：

| 韌體 shield | 用途 |
| --- | --- |
| `charybdis_dongle` | 中央端，透過 BLE 連接左右半部，透過 USB 輸出鍵盤及滑鼠，提供 Studio USB 連線 |
| `charybdis_left` | 左半部周邊端 |
| `charybdis_right` | 右半部周邊端，傳送按鍵、旋鈕與 PMW3610 軌跡球資料 |
| `settings_reset` | 清除控制器的配對與儲存設定 |

Nano35 的 35 鍵實體配置沿用現有 shield。`config/charybdis.keymap` 的左右上方三列依照
[Keyball39 V4.0-hotFixed](https://github.com/HyDroGen2528/zmk-dongle-keyball39/blob/V4.0-hotFixed/config%2Fkeyball39.keymap)
配置，套用以下簡化：防誤觸左鍵改為一般左鍵；巨集、拖曳鎖定、穿透與 Boot 均改為無動作，不移植來源的其他自訂功能。
保留標準按鍵、滑鼠按鍵、主列長按修飾鍵與 I／O 的長按切層（L4／L5）。
原有五顆拇指鍵與其既有非穿透綁定保留；L8～L10 也改為無動作，供 Studio 日後配置。
L5、L7 的上方三列全為無動作；L6 僅保留一般滑鼠三鍵，不會啟用來源的精細操作模式。
目前拇指鍵可進入 L1／L2，I／O 可進入 L4／L5；L3、L6、L7 未安排實體切層鍵，可在 DYA Studio 自行設定。
第 1 層的軌跡球捲動改由 dongle 處理，比例為 70 單位／格；其他層維持游標移動。
RGB 保留在左右半部，dongle 使用沒有實體輸出的 RGB 狀態裝置，讓全域 RGB 控制仍可轉送至兩側。
PMW3610 驅動固定版本；`config/charybdis.c` 為舊版驅動提供周邊端相容處理，分層邏輯只在 dongle 執行。

GitHub Actions 會依 `build.yaml` 建置以上四種韌體。Studio 的 `studio-rpc-usb-uart` snippet 只套用在 dongle。
下載方式：登入 GitHub，開啟本專案的 **Actions → Build Nano35 dongle firmware**，選擇 `charybdis-Nano35` 分支最近一次成功的執行，在 **Artifacts** 下載 `firmware` ZIP。
其中包含左右半部、dongle 與設定重置的四個 UF2，檔名格式為 `<shield>-nice_nano_v2-zmk.uf2`。
另附 `charybdis_right-debug-nice_nano_v2-zmk.uf2` 與 `charybdis_dongle-debug-nice_nano_v2-zmk.uf2`，供連線異常時收集 USB 序列埠記錄；正常使用請選沒有 `debug` 的檔案。

在已設定好的 DYA 雲端環境可執行：

```bash
bash /workspace/.zmk-env/build-dya.sh all
```

輸出位於 `/workspace/.zmk-env/dya-workspace/firmware/`，工具、建置檔及記錄均留在專案外。

首次從原本的兩半模式改為 dongle 模式時：

1. 左半部、右半部、dongle 三個控制器都先刷入 `settings_reset`，等待韌體啟動並清除設定。這也會清除已儲存的 Studio 改鍵。
2. 再分別刷入 `charybdis_left`、`charybdis_right`、`charybdis_dongle` 對應的 UF2。
3. 將 dongle 接上電腦 USB，讓左右半部開機並靠近 dongle，自動建立 BLE 配對。Studio 也改連線至 dongle。

左右半部的 USB 保留刷機用途；日常按鍵及軌跡球輸出由 dongle 負責。
雲端建置可確認韌體可編譯；BLE 配對、RGB、旋鈕及軌跡球仍須在實體鍵盤確認。

若只有右半部沒有反應，請先確認 dongle 與右半部來自同一次建置。
軌跡球的 split input ID 在兩側都必須為 `1`；ZMK v0.3 的探索程式會把 ID `0` 當成尚未完成的通道。
修正 ID 後需同時更新 dongle 與右半部，並重新開機；不需要因為這個 ID 變更再次清除配對。
若仍無反應，可暫時刷入各自的 `debug` UF2，右半部也以 USB 資料線接到電腦。
在序列埠工具以 115200、8N1 開啟相應的 USB COM 埠，觀察右手開機、與 dongle 連線，以及按鍵／軌跡球操作的記錄。
若同時接上 dongle 的 Studio 與記錄序列埠，請選會輸出 ZMK 記錄的那個埠。
除錯版本保留原本的中央／周邊角色，開啟斷言與堆疊溢位偵測；它不會讓右半部獨立成為 USB 鍵盤。

## DYA Studio 擴充

此分支已改用開發指南指定的 `cormoran/zmk`（`main+dya`）、作者的 Zephyr 4.1，以及 SDK 0.17.0。
`config/west.yml` 將核心與所有擴充模組固定在確切提交；GitHub Actions 也使用同一份 ZMK 核心的建置 workflow。
硬體仍為 nice!nano v2，新版板型名稱是 `nice_nano@2.0.0/nrf52840/zmk`，下載的 UF2 檔名維持不變。

已啟用的介面包含：

- Keymap：快速讀取與原始配置比對、即時按鍵顯示，以及執行時 Macro、Combo、Tap Dance。
- Trackball：游標與捲動兩個 runtime processor，可即時調整軟體感度、旋轉、反向與相關處理；第 1 層預設仍為捲動，比例 1/70。
- Connection：BLE profile、USB／BLE 優先順序、依連線或 OS 指定預設層。
- Settings：共用設定、進階設定，以及周邊端的設定 relay。
- Diagnostics：裝置資訊、左右按鍵掃描診斷、dongle 的 Watchdog 記錄與 Devtool 日誌。

以 Chrome／Edge 開啟 https://studio.dya.cormoran.works ，透過 USB 選擇 dongle 的 Studio 序列埠。
目前維持原專案的未鎖定設定；新版鍵位不再包含 Q 長按切層及 W 解鎖。若日後開啟 Studio 鎖定，須先安排解鎖鍵。
如果先前曾在 Studio 儲存改鍵，刷入新的 keymap 後，儲存設定可能仍會覆蓋韌體預設；
請先備份，再使用 Studio 的 **Restore Stock Settings** 套用新預設。單純更新鍵位不必清除三個控制器的 BLE 配對。

為保留目前已可用的感測器通訊流程，舊 PMW3610 驅動已納入 `config/drivers/pmw3610/`，使用獨立 binding 名稱避免與 Zephyr 4.1 衝突。
這一版沒有接上 DYA 的感測器硬體 CPI、暫存器或影像擷取 RPC；這些功能需改用另一套驅動並實機驗證。
上游 Watchdog relay 註明只保證單一周邊端，因此目前關閉該 relay，保留 dongle 本機的 Watchdog UI；左右半部仍各自記錄故障資訊。

這次包含 ZMK／Zephyr 與 Studio 儲存格式升級，請先備份原本的改鍵設定，三個控制器都使用同一次 DYA 建置的韌體。
建議依前述 settings_reset 流程清除舊設定後再刷入；這會清除配對及 Studio 改鍵。
原版韌體可從 GitHub 的 `155e00c` 提交及對應 Actions 成品回復。

開發指南原始資料：https://github.com/cormoran/dya-studio/blob/main/src/content/developerGuide.ts
