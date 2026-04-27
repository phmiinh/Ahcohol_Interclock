# Alcohol Interlock System

## Periodic Retest While Running

Sau khi vao `RUNNING`, firmware lap lich retest dinh ky. Production target la `30 phut`; khi `kDemoMode = true`, chu ky duoc rut xuong `30 giay` de demo duoc trong Wokwi.

Khi den han, state chuyen sang `RETEST_REQUIRED`: servo van mo khoa, LED xanh + vang sang, buzzer beep ngan theo chu ky va OLED yeu cau nhan `TEST` lai. Khi nhan `TEST`, firmware vao `RETEST_SAMPLING` va lay mau non-blocking trong khi servo van mo. Neu retest PASS thi quay lai `RUNNING` va lap lich chu ky moi; neu FAIL thi ve `FAIL_LOCKED`; neu het grace window ma khong test thi vao `ERROR_LOCKED` voi fault `RETEST_TIMEOUT`.

Telemetry `AI_JSON` co them cac field: `retestRequired`, `retestIntervalMs`, `retestRemainingMs`, `retestGraceRemainingMs`, `retestOverdueMs`, `retestDueToTestMs`, `retestToResultMs`, `runningSessionMs`, `retestCycleCount`.

Đây là prototype đồ án môn Xây dựng hệ thống nhúng dùng `ESP32` để kiểm tra nồng độ cồn trước khi cho phép khởi động. Hệ thống mô phỏng khóa liên động bằng `servo SG90`, hiển thị trạng thái qua `OLED SSD1306`, cảnh báo bằng `buzzer` và `LED đỏ/xanh/vàng`, nhận thao tác từ hai nút `TEST` và `START`.

Repo này được chốt theo đúng hướng demo hiện tại:
- `ESP32 Arduino + PlatformIO`
- `Wokwi local` để mô phỏng firmware
- `potentiometer` thay `MQ3` trong Wokwi
- `servo` là cơ cấu chấp hành chính
- `dashboard` realtime là phần mở rộng, không thay đổi logic nhúng cốt lõi

Firmware source of truth là toàn bộ thư mục `src/`. Thư mục `.pio/` chỉ là build output local của PlatformIO và không phải source của dự án.

## Mục tiêu demo

Ba flow chính cần chạy ổn định:

1. `Boot / Preheat`
2. `PASS rồi START`
3. `FAIL rồi không cho START`

Flow phụ để demo nhanh:
- Khi đang `RUNNING`, nhấn `START` thêm lần nữa để quay về trạng thái khóa.

## Tính năng hiện có

- State machine rõ ràng với các trạng thái `PREHEAT`, `STANDBY_LOCKED`, `SAMPLING`, `PASS_READY`, `FAIL_LOCKED`, `RUNNING`, `RETEST_REQUIRED`, `RETEST_SAMPLING`, `ERROR_LOCKED`
- Sampling ADC và beep PASS đều chạy non-blocking bằng `millis()`
- Sampling chỉ lấy tối đa `1 mẫu / 1 vòng update`, không còn cơ chế dồn mẫu khi `loop()` bị trễ
- Nút `TEST` có thêm `GPIO interrupt`, còn debounce vẫn giữ trong software để an toàn và dễ giải thích
- Buzzer dùng `LEDC hardware timer` của ESP32 để phát beep 2 kHz ổn định hơn
- Safe state khi fault cứng: servo khóa, không cho START, log lỗi rõ ràng
- Cảnh báo sensor thực tế hơn: nếu ADC bị ghim gần `0` hoặc `4095` quá lâu thì phát `sensor warning`, không khóa hệ thống chỉ vì một lần đọc lạ
- Nếu toàn bộ một phiên sampling đều bị ghim sát rail ADC thì vào hard fault `SENSOR_TIMEOUT`
- Telemetry `AI_JSON` giữ tương thích với dashboard hiện có
- Serial log và telemetry đã có thêm metric để viết báo cáo:
  - `test_to_result_ms`
  - `pass_ready_to_unlock_ms`
  - `start_to_unlock_ms`
  - `sampleStdDev`
  - `consecutiveFailCount`

## Pin mapping chuẩn

`START` đã được chốt thống nhất là `GPIO16` trên toàn bộ repo.

| Chức năng | GPIO |
|---|---|
| MQ3 giả lập / ADC | `GPIO34` |
| Servo SG90 | `GPIO15` |
| Buzzer | `GPIO13` |
| LED đỏ | `GPIO25` |
| LED xanh | `GPIO26` |
| LED vàng | `GPIO27` |
| Nút TEST | `GPIO14` |
| Nút START | `GPIO16` |
| OLED SDA | `GPIO21` |
| OLED SCL | `GPIO22` |

## Cấu trúc repo

- `src/main.cpp`: entry point cho PlatformIO
- `src/config.h`: pin map, timing, threshold, flag cấu hình
- `src/state_machine.*`: FSM chính, sampling non-blocking, buzzer, fault handling
- `src/io_devices.*`: servo, buzzer, LED, button, ADC, OLED init
- `src/ui_oled.*`: render giao diện OLED
- `src/telemetry.*`: log Serial và protocol `AI_JSON`
- `dashboard/`: Node.js backend + web dashboard realtime
- `diagram.json`: sơ đồ Wokwi
- `platformio.ini`: cấu hình build firmware
- `wokwi.toml`: trỏ firmware local cho Wokwi VS Code
- `roadmap.md`: hướng dẫn chạy firmware, Wokwi, dashboard và checklist demo

## Chạy nhanh

1. Build firmware:

```powershell
pio run
```

2. Mở `diagram.json` trong VS Code rồi chạy `Wokwi: Start Simulator`.
3. Nếu cần dashboard:

```powershell
npm install
npm run start
```

Hoặc chạy UI mock:

```powershell
npm run mock
```

## Ghi chú prototype

- Wokwi không dùng `MQ3` thật, mà dùng `potentiometer` để mô phỏng mức ADC.
- Trong Wokwi, `TEST` và `START` đang được mô phỏng bằng `pushbutton thường + điện trở kéo lên 10k` để giả lập hành vi `OUT active-LOW` của module nút 3 chân.
- Trên phần cứng thật, firmware mặc định coi hai input này là `active LOW`: idle `HIGH`, nhấn `LOW`.
- `kDemoMode`, `kPreheatMs`, `kSampleCount` và `kAlcoholAdc` là tham số demo cho Wokwi và cho phần báo cáo, không phải calibration cuối cho `MQ3` thật.
- `TEST` đang dùng interrupt chỉ để set flag; debounce và logic nghiệp vụ vẫn chạy ở `loop()`, nên an toàn cho demo và đúng tinh thần embedded.
- Dashboard tối ưu cho `ESP32 thật qua USB serial`; dashboard không đọc trực tiếp từ cửa sổ Wokwi.
- Nếu module nút của bạn là `active HIGH`, đổi `config::buttons::kActiveHigh` trong [src/config.h](src/config.h) sang `true`.

## Known Hardware Assumptions

- Firmware mặc định cho mô hình input `active LOW`
- Bias nội của ESP32 đang bật mặc định để giữ idle ổn định:
  - `pulldown` khi `active HIGH`
  - `pullup` khi `active LOW`
- `START` được chốt là `GPIO16`
- Servo nên dùng nguồn đủ dòng và chung `GND` với ESP32
- Nếu chuyển sang `MQ3` thật, cần calibration lại threshold, thời gian warm-up và kiểm soát điện áp ADC vào `GPIO34`

Chi tiết cách build, mô phỏng, chạy dashboard, wiring và checklist demo nằm trong [roadmap.md](roadmap.md).
