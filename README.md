# Alcohol Interlock System

## Tổng Quan

`Alcohol Interlock System` là prototype hệ thống nhúng dùng `ESP32` để kiểm tra điều kiện an toàn trước khi cho phép mở khóa mô hình khởi động xe. Hệ thống mô phỏng cảm biến nồng độ cồn bằng tín hiệu ADC, xử lý kết quả theo máy trạng thái hữu hạn, điều khiển servo làm cơ cấu khóa, hiển thị trạng thái trên OLED và cảnh báo bằng LED/buzzer.

Dự án hiện ưu tiên ba mục tiêu:

- Mô phỏng rõ bài toán khóa liên động trước khi khởi động.
- Có firmware đủ sạch để giải thích trong báo cáo môn Xây dựng hệ thống nhúng.
- Chạy ổn định trên Wokwi và có thể triển khai lên phần cứng thật với các lưu ý rõ ràng.

## Chức Năng Chính

- Khởi động hệ thống và đưa xe về trạng thái khóa an toàn.
- Preheat cảm biến trước khi cho phép đo.
- Nhấn `TEST` để lấy mẫu ADC và đánh giá PASS/FAIL.
- Chỉ khi PASS mới cho phép nhấn `START`.
- Nhấn `START` hợp lệ thì servo mở khóa và hệ thống vào `RUNNING`.
- Khi đang `RUNNING`, hệ thống yêu cầu retest định kỳ.
- Nếu retest PASS thì tiếp tục chạy.
- Nếu retest FAIL thì khóa lại.
- Nếu đến hạn retest nhưng người dùng không kiểm tra lại trong grace window thì vào trạng thái lỗi an toàn; riêng lỗi này cho phép nhấn `TEST` để kiểm tra lại từ trạng thái khóa.
- Log Serial gọn, tập trung vào trạng thái, hành động, kết quả và lỗi.
- Dashboard realtime có sẵn nhưng đang tắt telemetry mặc định để Serial Monitor Wokwi dễ theo dõi.

## Luồng Hoạt Động Hiện Tại

Luồng khởi động:

```text
BOOT -> PREHEAT -> STANDBY_LOCKED
```

Luồng kiểm tra đạt:

```text
STANDBY_LOCKED -> TEST -> SAMPLING -> PASS_READY -> START -> RUNNING
```

Luồng kiểm tra không đạt:

```text
STANDBY_LOCKED -> TEST -> SAMPLING -> FAIL_LOCKED
```

Luồng retest khi đang chạy:

```text
RUNNING -> RETEST_REQUIRED -> TEST -> RETEST_SAMPLING
```

Kết quả retest:

```text
RETEST_SAMPLING -> RUNNING      nếu PASS
RETEST_SAMPLING -> FAIL_LOCKED  nếu FAIL
RETEST_REQUIRED -> ERROR_LOCKED nếu timeout
ERROR_LOCKED -> TEST -> SAMPLING nếu fault là RETEST_TIMEOUT
```

## Máy Trạng Thái

Firmware hiện có các trạng thái:

| Trạng thái | Ý nghĩa |
|---|---|
| `PREHEAT` | Làm nóng/ổn định cảm biến trước khi đo |
| `STANDBY_LOCKED` | Xe đang khóa, chờ nhấn TEST |
| `SAMPLING` | Đang lấy mẫu ADC cho lần kiểm tra chính |
| `PASS_READY` | Kết quả đạt, chờ người dùng nhấn START |
| `FAIL_LOCKED` | Kết quả không đạt, xe vẫn khóa |
| `RUNNING` | Servo đã mở khóa, hệ thống đang chạy |
| `RETEST_REQUIRED` | Đến hạn kiểm tra lại khi đang chạy |
| `RETEST_SAMPLING` | Đang lấy mẫu cho lần retest |
| `ERROR_LOCKED` | Lỗi an toàn, xe khóa; `RETEST_TIMEOUT` có thể nhấn TEST để kiểm tra lại |

## Cấu Hình Quan Trọng

Các tham số chính nằm trong [src/config.h](src/config.h).

| Tham số | Giá trị hiện tại | Ghi chú |
|---|---:|---|
| Board | `esp32dev` | PlatformIO ESP32 Dev Module |
| Framework | `Arduino` | Dễ mô phỏng và triển khai |
| Baud rate | `115200` | Serial Monitor |
| `kDemoMode` | `true` | Dùng timing demo |
| `kPreheatMs` | `10000 ms` | Preheat 10 giây trong demo |
| `kSampleCount` | `20` | Số mẫu mỗi lần đo |
| `kSampleTotalMs` | `2000 ms` | Cửa sổ lấy mẫu khoảng 2 giây |
| `kAlcoholAdc` | `2000` | Ngưỡng demo, cần hiệu chuẩn lại với MQ3 thật |
| `kRetestDemoMs` | `60000 ms` | Retest mỗi 1 phút trong demo |
| `kRetestProductionMs` | `1800000 ms` | Retest mỗi 30 phút trong production |
| `kRetestGraceMs` | `15000 ms` demo | Quá thời gian này sẽ safe-lock |
| `kActiveHigh` | `false` | Nút mặc định active-LOW |

## Phần Cứng Và Pin Mapping

| Chức năng | GPIO ESP32 | Ghi chú |
|---|---:|---|
| ADC cảm biến/MQ3 mô phỏng | `GPIO34` | Wokwi dùng potentiometer |
| Servo khóa | `GPIO15` | Góc khóa `0`, góc mở `90` |
| Buzzer | `GPIO13` | Phát bằng LEDC hardware timer |
| LED đỏ | `GPIO25` | FAIL/ERROR |
| LED xanh | `GPIO26` | PASS/RUNNING |
| LED vàng | `GPIO27` | PREHEAT/SAMPLING/RETEST |
| Nút TEST | `GPIO14` | Active-LOW |
| Nút START | `GPIO16` | Trên Wokwi DevKit là `RX2` |
| OLED SDA | `GPIO21` | I2C |
| OLED SCL | `GPIO22` | I2C |

## Lưu Ý Về Nút Bấm

Firmware hiện cấu hình nút theo mô hình `active-LOW`:

- Không nhấn: tín hiệu `HIGH`.
- Nhấn: tín hiệu `LOW`.
- ESP32 dùng `INPUT_PULLUP` khi `kActiveHigh = false`.

Nếu module nút thật của bạn là active-HIGH, cần đổi:

```cpp
constexpr bool kActiveHigh = true;
```

Triệu chứng cấu hình sai polarity:

- Nhấn TEST không có phản ứng.
- Sau khi PASS, hệ thống tự nhảy sang RUNNING.
- START không được nhận dù wiring đúng.

## Buzzer Hiện Tại

- PASS sau khi test: kêu một tiếng ngắn.
- START hợp lệ từ `PASS_READY`: kêu một tiếng ngắn.
- FAIL: buzzer cảnh báo theo chu kỳ.
- `RETEST_REQUIRED`: buzzer dùng cùng pattern cảnh báo với FAIL và kêu lặp liên tục cho tới khi người dùng nhấn TEST, nhấn START để khóa lại, hoặc hết grace window.
- ERROR: hệ thống khóa an toàn, buzzer không spam liên tục.

## Cấu Trúc Repo

```text
src/
  main.cpp            Entry point PlatformIO
  config.h            Pin map, threshold, timing, feature flags
  app_types.h         State, fault, snapshot, helper string
  io_devices.*        Giao tiếp GPIO, ADC, servo, buzzer, OLED
  state_machine.*     FSM chính và business logic
  telemetry.*         Serial log và AI_JSON telemetry
  ui_oled.*           Render OLED

dashboard/
  server.js           Node.js server đọc serial và phát socket
  public/             Web dashboard

diagram.json          Sơ đồ Wokwi
wokwi.toml            Trỏ firmware Wokwi local
platformio.ini        Cấu hình PlatformIO
roadmap.md            Quy trình triển khai và kiểm thử
fine-tunning.md       Nhật ký tinh chỉnh sau retest
```

## Chạy Firmware Trên Wokwi

Build firmware:

```powershell
pio run
```

Sau khi build thành công, Wokwi dùng:

```text
.pio/build/esp32dev/firmware.bin
.pio/build/esp32dev/firmware.elf
```

Chạy mô phỏng:

1. Mở `diagram.json`.
2. Chạy lệnh `Wokwi: Start Simulator`.
3. Theo dõi Serial Monitor.

## Chạy Dashboard

Cài dependency:

```powershell
npm install
```

Liệt kê cổng serial:

```powershell
npm run ports
```

Chạy dashboard với ESP32 thật:

```powershell
npm run start
```

Chạy mock dashboard:

```powershell
npm run mock
```

Mở trình duyệt tại:

```text
http://localhost:3030
```

Lưu ý: firmware đang tắt `kEnableDashboardProtocol` mặc định để Serial Monitor Wokwi không bị spam `AI_JSON`. Nếu muốn dashboard realtime đọc dữ liệu từ board thật, bật lại `kEnableDashboardProtocol = true` trong [src/config.h](src/config.h).

## Checklist Demo

1. Build `pio run` thành công.
2. Wokwi nạp được firmware.
3. Boot vào `PREHEAT`.
4. Sau 10 giây vào `STANDBY_LOCKED`.
5. Nhấn TEST khi ADC thấp hơn threshold.
6. Hệ thống vào `SAMPLING` khoảng 2 giây.
7. Kết quả PASS, OLED hiện `Press START`.
8. Nhấn START, servo mở và state vào `RUNNING`.
9. Sau 60 giây demo, hệ thống vào `RETEST_REQUIRED`.
10. Buzzer nhắc retest.
11. Nhấn TEST để retest.
12. Retest PASS thì tiếp tục RUNNING.
13. Retest FAIL thì khóa lại.
14. Không retest trong grace window thì vào `ERROR_LOCKED`; nhấn TEST để kiểm tra lại từ trạng thái khóa.

## Lưu Ý Khi Lắp Phần Cứng Thật

- Servo nên dùng nguồn 5V riêng đủ dòng và chung GND với ESP32.
- Không cấp servo từ chân 3V3.
- Nếu ESP32 reset khi servo chạy, ưu tiên kiểm tra nguồn servo.
- Chân START thật nối vào `GPIO16`; một số board in là `D16` hoặc `RX2`.
- Chân TEST thật nối vào `GPIO14`.
- Với nút active-LOW, đo thực tế nên thấy: không nhấn khoảng 3.3V, nhấn khoảng 0V.
- MQ3 thật cần warm-up và hiệu chuẩn threshold, không dùng nguyên `kAlcoholAdc = 2000` như giá trị cuối cùng.

## Trạng Thái Hiện Tại

Firmware đã ổn cho demo chính:

- TEST và START đều xử lý qua interrupt flag kết hợp debounce phần mềm.
- `PASS_READY` không tự mở khóa nếu START đang active sẵn.
- START có guard để tránh một lần bấm bị tính thành mở rồi khóa lại.
- Retest demo mỗi 1 phút sau khi vào `RUNNING`.
- Buzzer có phản hồi cho PASS, START và retest.
