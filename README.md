# Alcohol Interlock System

Prototype đồ án môn Xây dựng hệ thống nhúng dùng `ESP32` để kiểm tra nồng độ cồn trước khi cho phép khởi động. Hệ thống mô phỏng khóa liên động bằng `servo SG90`, hiển thị trạng thái qua `OLED SSD1306`, cảnh báo bằng `buzzer` và `LED đỏ/xanh/vàng`, nhận thao tác từ hai nút `TEST` và `START`.

Repo này giữ đúng hướng triển khai cho demo:
- `ESP32 Arduino + PlatformIO`
- `Wokwi local` để mô phỏng firmware
- `potentiometer` thay `MQ3` trong Wokwi
- `servo` là chấp hành chính
- `dashboard` realtime là phần mở rộng, không thay đổi logic nhúng cốt lõi

## Mục tiêu demo

Ba flow chính cần chạy ổn định:

1. `Boot / Preheat`
2. `PASS rồi START`
3. `FAIL rồi không cho START`

Flow phụ để demo thuận tiện:
- Khi đang `RUNNING`, nhấn `START` thêm lần nữa để quay về trạng thái khóa.

## Tính năng hiện có

- State machine rõ ràng với các trạng thái `PREHEAT`, `STANDBY_LOCKED`, `SAMPLING`, `PASS_READY`, `FAIL_LOCKED`, `RUNNING`, `ERROR_LOCKED`
- Sampling ADC và beep PASS đã chuyển sang non-blocking bằng `millis()`
- Safe state khi lỗi: khóa xe, không cho START, giữ log lỗi rõ ràng
- Telemetry `AI_JSON` vẫn tương thích với dashboard hiện tại
- OLED, LED, buzzer và servo được tách module để dễ đọc và dễ giải thích khi báo cáo

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
- `src/state_machine.*`: FSM chính, non-blocking sampling, pass beep, fault handling
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
- `ALCOHOL_THRESHOLD` hiện là ngưỡng demo, không phải giá trị hiệu chuẩn cho phần cứng thật.
- Dashboard tối ưu cho `ESP32 thật qua USB serial`; dashboard không đọc trực tiếp từ cửa sổ Wokwi.
- Nếu module nút của bạn là `active LOW`, đổi `config::buttons::kActiveHigh` trong [src/config.h](src/config.h).

Chi tiết cách build, mô phỏng, chạy dashboard, wiring và checklist demo nằm trong [roadmap.md](roadmap.md).
