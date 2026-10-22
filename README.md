# Nano35 Charybdis：SavingPower dongle 模式

`CharybdisNano35-SavingPower` 由 `charybdis-Nano35` 的 `0041648` 建立，使用三個 `nice_nano_v2`：

| 韌體 shield | 用途 |
| --- | --- |
| `charybdis_dongle` | 中央端，透過 BLE 連接左右半部，透過 USB 輸出鍵盤及滑鼠，提供 Studio USB 連線 |
| `charybdis_left` | 左半部周邊端 |
| `charybdis_right` | 右半部周邊端，傳送按鍵、旋鈕與 PMW3610 軌跡球資料 |
| `settings_reset` | 清除控制器的配對與儲存設定 |

本分支的省電調整：

- 活動期間的電量量測／更新週期由 60 秒改為 300 秒。沿用原本的閒置停測、恢復活動立即量測；此項主要減少連續使用時的量測次數，省電效益較小。
- 左半部的外部 VCC 預設關閉，開機由 GPIO hog 將 P0.13 設為低；可在 DYA Studio 的進階設定、左半部來源下，修改 `cormoran_custom_settings / nano35_left_vcc_enabled` 並儲存。舊版外部電源驅動保持停用，避免其舊設定覆蓋新設定。
- 右半部保留 VCC 供電，避免軌跡球因共用電源而失效。若右手燈條與軌跡球共用 VCC，韌體無法單獨切斷燈條的待機供電。
- 僅產生一般版韌體與設定重置檔。除錯版本仍可從原本的 `charybdis-Nano35` 分支取得。

本版保留 +8 dBm 發射功率、BLE 連線參數與 30 秒 Idle。深度睡眠支援已編入，逾時預設為 0（不自動深睡）；可在 Studio 的 Settings 設定各控制器的睡眠時間。
PMW3610 預設 CPI 400、交換 XY 並反向兩軸（等同原本方向）、最短回報間隔 8 ms。REST 預設依序為 RUN 128 ms、REST1 取樣 40 ms／降階 9600 ms、REST2 取樣 100 ms／降階 12800 ms、REST3 取樣 500 ms，均可在 Studio 調整。
按鍵配置沿用原分支。左半部若自行加裝需要 VCC 的硬體，可使用上述設定開啟供電；RGB 韌體功能仍維持移除。
實際省電量取決於 VCC 接線與外接負載；僅能透過硬體電流量測確認，不能由編譯結果推算續航增幅。

Nano35 的 35 鍵實體配置沿用現有 shield。`config/charybdis.keymap` 的左右上方三列依照
[Keyball39 V4.0-hotFixed](https://github.com/HyDroGen2528/zmk-dongle-keyball39/blob/V4.0-hotFixed/config%2Fkeyball39.keymap)
配置，套用以下簡化：防誤觸左鍵改為一般左鍵；巨集、拖曳鎖定、穿透與 Boot 均改為無動作，不移植來源的其他自訂功能。
保留標準按鍵、滑鼠按鍵、主列長按修飾鍵與 I／O 的長按切層（L4／L5）。
原有五顆拇指鍵與其既有非穿透綁定保留；L8～L10 也改為無動作，供 Studio 日後配置。
L5、L7 的上方三列全為無動作；L6 僅保留一般滑鼠三鍵，不會啟用來源的精細操作模式。
目前拇指鍵可進入 L1／L2，I／O 可進入 L4／L5；L3、L6、L7 未安排實體切層鍵，可在 DYA Studio 自行設定。
第 1 層的軌跡球捲動改由 dongle 處理，比例為 70 單位／格；其他層維持游標移動。
RGB 燈效功能已移除，包含左右半部燈條、專用 SPI 匯流排、dongle 虛擬燈條及相關韌體設定。
PMW3610 使用固定提交的 DYA RPC 驅動，保留 Nano35 共用 MOSI／MISO 資料腳的傳輸方式；游標／捲動分層仍在 dongle 執行。

GitHub Actions 會依 `build.yaml` 建置以上四種韌體。Studio 的 `studio-rpc-usb-uart` snippet 只套用在 dongle。
下載方式：登入 GitHub，開啟本專案的 **Actions → Build Nano35 SavingPower firmware**，選擇 `CharybdisNano35-SavingPower` 分支最近一次成功的執行，在 **Artifacts** 下載 `firmware` ZIP。
其中包含左右半部、dongle 與設定重置的四個 UF2，檔名格式為 `<shield>-nice_nano_v2-zmk.uf2`。
更新此省電版時，刷入同次建置的左右半部與 dongle 即可，不必清除 BLE 配對或重置 Studio 改鍵。

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
雲端建置可確認韌體可編譯；BLE 配對、旋鈕及軌跡球仍須在實體鍵盤確認。

若只有右半部沒有反應，請先確認 dongle 與右半部來自同一次建置。
軌跡球的 split input ID 在兩側都必須為 `1`；ZMK v0.3 的探索程式會把 ID `0` 當成尚未完成的通道。
修正 ID 後需同時更新 dongle 與右半部，並重新開機；不需要因為這個 ID 變更再次清除配對。
若仍無反應，可從原本 `charybdis-Nano35` 分支的建置取得右半部與 dongle 的 `debug` UF2，暫時刷入後讓右半部也以 USB 資料線接到電腦。
在序列埠工具以 115200、8N1 開啟相應的 USB COM 埠，觀察右手開機、與 dongle 連線，以及按鍵／軌跡球操作的記錄。
若同時接上 dongle 的 Studio 與記錄序列埠，請選會輸出 ZMK 記錄的那個埠。
除錯版本保留原本的中央／周邊角色，開啟斷言與堆疊溢位偵測；它不會讓右半部獨立成為 USB 鍵盤。

## DYA Studio 擴充

此分支已改用開發指南指定的 `cormoran/zmk`（`main+dya`）、作者的 Zephyr 4.1，以及 SDK 0.17.0。
`config/west.yml` 將核心與所有擴充模組固定在確切提交；GitHub Actions 也使用同一份 ZMK 核心的建置 workflow。
硬體仍為 nice!nano v2，新版板型名稱是 `nice_nano@2.0.0/nrf52840/zmk`，下載的 UF2 檔名維持不變。

已啟用的介面包含：

- Keymap：快速讀取與原始配置比對、即時按鍵顯示，以及執行時 Macro、Combo、Tap Dance。
- Trackball：游標／捲動 runtime processor 與 PMW3610 硬體 CPI、軸向、REST 省電參數、暫存器、診斷及影像擷取／串流；第 1 層預設仍為捲動，比例 1/70。
- Connection：BLE profile、USB／BLE 優先順序、依連線或 OS 指定預設層。
- Settings：共用設定、進階設定、可調的閒置／深睡時間、左手 VCC，以及周邊端的設定 relay。
- Diagnostics：裝置資訊、左右按鍵掃描診斷、dongle 與左右半部的 Watchdog 記錄讀取／刪除，以及 Devtool 日誌。
- Layout：35 鍵配置之外，加入右手軌跡球及旋鈕的示意位置。只保留啟用的右旋鈕，因此其 sensor index 為 0；先前若曾在 Studio 自訂旋鈕綁定，請備份並重新對應。

以 Chrome／Edge 開啟 https://studio.dya.cormoran.works ，透過 USB 選擇 dongle 的 Studio 序列埠。
目前維持原專案的未鎖定設定；新版鍵位不再包含 Q 長按切層及 W 解鎖。若日後開啟 Studio 鎖定，須先安排解鎖鍵。
如果先前曾在 Studio 儲存改鍵，刷入新的 keymap 後，儲存設定可能仍會覆蓋韌體預設；
請先備份，再使用 Studio 的 **Restore Stock Settings** 套用新預設。單純更新鍵位不必清除三個控制器的 BLE 配對。

PMW3610 驅動固定為 `cormoran/zmk-driver-pmw3610-with-custom-studio-rpc@5c34ea0`。
`config/patches/` 的相容修正在建置目錄套用到來源副本，不修改下載的依賴：

- SPI 保持 GPIO CS 與分開的送位址／讀資料階段，沿用 Nano35 既有共用資料腳接線；一般動作採逐暫存器讀取，影像讀取以最多 240 bytes 分段並全程保持 CS。
- 動作讀取與 Studio 操作共用 mutex；關閉 force-awake 時清除強制清醒位元。
- PMW3610 與 Watchdog 的指定來源請求直接送至當前 BLE slot，避免廣播造成錯誤回覆或誤刪另一半部的記錄；僅感測器自動探索使用廣播。
- Watchdog 回覆使用佇列保留連續的 incident chunks，避免未執行的工作資料被下一筆覆蓋。

Studio 的來源 1／2 是連線 slot，不保證固定代表左／右；請依裝置資訊與感測器探索結果選擇。左半部不建立空的感測器回覆介面。
調整 PMW3610 設定時選擇感測器 `right` 所在的周邊來源；左手 VCC 在左半部來源的進階設定 `cormoran_custom_settings / nano35_left_vcc_enabled`。
影像擷取會暫停感測器游標輸出，完成或停止後驅動重新初始化恢復；這是感測器偵錯操作，日常使用請停止串流。
深睡預設關閉。自行開啟後可按鍵喚醒，需重新建立 BLE 連線；軌跡球不作為深睡喚醒來源。USB 供電時沿用 ZMK 的不自動深睡規則。
編譯與主機端檢查只能確認程式與協定邏輯，實際 SPI 電氣訊號、影像、BLE 轉送、喚醒與耗電仍須在實體鍵盤確認。
可從 `91e8639` 與其 Actions 成品回復到先前省電版；不用為這次功能擴充清除 BLE 配對。

若從 `155e00c` 等舊版升級到 DYA，包含 ZMK／Zephyr 與 Studio 儲存格式變更，請先備份改鍵，三個控制器都使用同一次 DYA 建置的韌體。
這種跨版本升級建議依前述 settings_reset 流程清除舊設定；這會清除配對及 Studio 改鍵。由 `91e8639` 更新本版則不需要 settings_reset。
原版韌體可從 GitHub 的 `155e00c` 提交及對應 Actions 成品回復。

開發指南原始資料：https://github.com/cormoran/dya-studio/blob/main/src/content/developerGuide.ts
