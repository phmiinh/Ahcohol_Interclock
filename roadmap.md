# Roadmap Vận Hành Dự Án

File này tập trung vào cách build, cách demo và cách nối firmware với dashboard. `README.md` trả lời câu hỏi dự án là gì; file này trả lời câu hỏi chạy như thế nào.

Source of truth cho firmware là `src/*`. Thư mục `.pio/` chỉ là output build local của PlatformIO.

## 1. Khi nào dùng đường chạy nào

| Nhu cầu | Cách chạy phù hợp |
|---|---|
| Demo logic nhúng trên mô phỏng | `PlatformIO + Wokwi local` |
| Demo telemetry realtime với thiết bị thật | `ESP32 thật + USB serial + dashboard` |
| Test riêng UI dashboard khi chưa có board | `npm run mock` |

## 2. Yêu cầu môi trường

Bạn cần có:

- `VS Code`
- extension `PlatformIO IDE`
- extension `Wokwi for VS Code`
- internet và license Wokwi cho VS Code
- `Node.js` nếu muốn chạy dashboard

Toolchain hiện tại của repo:

- board: `esp32dev`
- framework: `arduino`
- platform: `espressif32@6.3.2`
- serial monitor: `115200`

## 3. Cấu trúc quan trọng sau refactor

- `src/main.cpp`: entry point
- `src/config.h`: nơi sửa pin, threshold, polarity nút
- `src/state_machine.*`: luồng nghiệp vụ chính
- `src/io_devices.*`: giao tiếp phần cứng
- `src/ui_oled.*`: màn hình OLED
- `src/telemetry.*`: log và `AI_JSON`
- `diagram.json`: sơ đồ mô phỏng
- `wokwi.toml`: map firmware local cho Wokwi
- `dashboard/server.js`: backend đọc serial và phát realtime
- `dashboard/public/*`: frontend dashboard

## 4. Build firmware local

Chạy ở thư mục project:

```powershell
pio run
```

Sau khi build thành công, Wokwi local sẽ dùng:

- `.pio/build/esp32dev/firmware.bin`
- `.pio/build/esp32dev/firmware.elf`

Nếu cần clean:

```powershell
pio run -t clean
pio run
```

## 5. Chạy Wokwi local trong VS Code

1. Build firmware bằng `pio run`
2. Mở file `diagram.json`
3. Nhấn `F1`
4. Chạy lệnh `Wokwi: Start Simulator`

Lưu ý:

- Wokwi local chỉ nạp firmware đã build sẵn từ PlatformIO
- Nếu đổi tên env trong `platformio.ini`, phải cập nhật lại `wokwi.toml`
- `TEST` hiện có thêm `GPIO interrupt`; Wokwi ESP32 vẫn mô phỏng được hướng này

## 6. Demo 3 flow chính

### Flow 1: Boot / Preheat

1. Start simulator
2. Quan sát `OLED` hiển thị warm-up
3. Quan sát `LED vàng` sáng
4. Chờ hết `kPreheatMs`
5. Hệ thống tự chuyển sang `STANDBY_LOCKED`

### Flow 2: PASS rồi START

1. Đưa `potentiometer` xuống mức thấp hơn threshold
2. Nhấn `TEST`
3. Quan sát:
   - OLED vào `SAMPLING`
   - Serial log tiến độ từng mẫu
   - log `average ADC`, `stddev`, `threshold`, `PASS`
   - metric `test_to_result_ms`
4. Hệ thống sang `PASS_READY`
5. Nhấn `START`
6. Servo mở khóa, state sang `RUNNING`
7. Quan sát thêm metric:
   - `pass_ready_to_unlock_ms`
   - `start_to_unlock_ms`

### Flow 3: FAIL rồi không cho START

1. Đưa `potentiometer` lên cao hơn threshold
2. Nhấn `TEST`
3. Quan sát:
   - OLED vào `SAMPLING`
   - Serial log `average ADC`, `stddev`, `threshold`, `FAIL`
   - `consecutiveFailCount` tăng
4. Hệ thống sang `FAIL_LOCKED`
5. `LED đỏ` sáng, `buzzer` cảnh báo
6. Nhấn `START` sẽ không mở khóa

### Flow phụ để demo nhanh

Khi đang ở `RUNNING`, nhấn `START` lần nữa để quay về `STANDBY_LOCKED`.

## 7. Chạy dashboard với ESP32 thật

Dashboard chỉ nên dùng khi có `ESP32 thật` cắm qua USB serial.

### Bước 1: cài dependency

```powershell
npm install
```

### Bước 2: tìm cổng serial

```powershell
npm run ports
```

### Bước 3: chạy server

Tự dò cổng:

```powershell
npm run start
```

Hoặc chỉ định cổng:

```powershell
node dashboard/server.js --serial COM5
```

### Bước 4: mở dashboard

Mở trình duyệt tại:

```text
http://localhost:3030
```

Dashboard sẽ hiển thị:

- connection status
- state machine
- live ADC
- sampled ADC
- threshold
- servo angle
- vehicle locked / unlocked
- buzzer status
- preheat countdown
- event history
- raw serial lines

## 8. Chạy dashboard ở mock mode

Khi chưa có board thật:

```powershell
npm run mock
```

Mock mode dùng để test UI và notification, không thay thế demo firmware.

## 9. Wiring và lưu ý phần cứng

Mapping hiện tại:

| Chức năng | GPIO |
|---|---|
| MQ3 analog / potentiometer | `GPIO34` |
| Servo SG90 | `GPIO15` |
| Buzzer | `GPIO13` |
| LED đỏ | `GPIO25` |
| LED xanh | `GPIO26` |
| LED vàng | `GPIO27` |
| TEST | `GPIO14` |
| START | `GPIO16` |
| OLED SDA | `GPIO21` |
| OLED SCL | `GPIO22` |

Lưu ý quan trọng:

- `START` đã chốt là `GPIO16`, không dùng `GPIO12`
- Wokwi dùng `potentiometer` thay `MQ3`
- Wokwi đang dùng `pushbutton thường + điện trở kéo xuống 10k` để mô phỏng tín hiệu `OUT active-HIGH`
- Phần cứng thật có thể dùng `module 3 chân` hoặc `nút rời + bias tương đương`
- Firmware bật bias nội cho input button để tránh idle bị float:
  - `pulldown` khi `active HIGH`
  - `pullup` khi `active LOW`
- `TEST` dùng `interrupt` để set flag sớm, nhưng debounce vẫn xử lý bằng software
- Buzzer dùng `LEDC hardware timer` để phát beep 2 kHz ổn định hơn
- Nếu module của bạn xuất `LOW` khi nhấn, đổi `config::buttons::kActiveHigh` trong `src/config.h`
- Nếu dùng `MQ3` thật và module cho analog `0-5V`, phải chia áp trước khi đưa vào `GPIO34`
- Servo nên dùng nguồn đủ dòng và chung `GND` với ESP32

## 10. Fault handling hiện tại

Sau refactor, hệ thống có fault handling tối thiểu nhưng thực tế hơn:

- `OLED init fail` -> vào `ERROR_LOCKED`, giữ safe state, log lỗi qua Serial
- `ADC bị ghim gần 0 hoặc 4095 quá lâu` -> phát `sensor warning`, log cảnh báo và gửi telemetry, nhưng không khóa chết hệ thống chỉ vì một lần đọc bất thường
- `toàn bộ phiên sampling đều stuck near rail` -> vào hard fault `SENSOR_TIMEOUT`, vì đây là lỗi sensor/wiring trong đúng thời điểm ra quyết định PASS/FAIL
- Safe state luôn là:
  - servo khóa
  - không cho START
  - telemetry vẫn giữ được nếu Serial còn hoạt động

## 11. Metric nên dùng trong báo cáo

Serial log và `AI_JSON` hiện đã có sẵn các chỉ số sau:

- `test_to_result_ms`: thời gian từ lúc nhấn `TEST` đến khi có kết quả PASS/FAIL
- `pass_ready_to_unlock_ms`: thời gian chờ trong `PASS_READY` trước khi người dùng nhấn `START`
- `start_to_unlock_ms`: độ trễ từ lúc nhấn `START` đến khi servo mở khóa
- `sampleStdDev`: độ lệch chuẩn đơn giản của các mẫu ADC trong một phiên đo
- `consecutiveFailCount`: số lần FAIL liên tiếp trước khi PASS
- `retestRemainingMs`: thoi gian con lai truoc khi yeu cau rolling retest khi dang `RUNNING`
- `retestDueToTestMs`: thoi gian tu luc he thong yeu cau retest den luc nguoi dung nhan `TEST`
- `retestToResultMs`: thoi gian tu luc nhan `TEST` trong retest den khi co ket qua PASS/FAIL

## 12. Checklist test trước khi demo

- `pio run` build thành công
- Wokwi local start được từ `diagram.json`
- `PREHEAT -> STANDBY_LOCKED` chuyển đúng
- Nhánh `PASS -> START -> RUNNING` chạy đúng
- Nhánh `FAIL -> START không có hiệu lực` chạy đúng
- Serial log có:
  - tiến độ lấy mẫu
  - average ADC
  - stddev
  - threshold
  - PASS/FAIL
  - các metric latency
- Dashboard vẫn nhận được `AI_JSON` nếu test với board thật hoặc mock mode

## Update: Running Periodic Retest

Tinh nang moi: sau khi `PASS -> START -> RUNNING`, firmware bat dau dem nguoc retest. Production target la `30 phut`; trong `kDemoMode`, gia tri nay la `30 giay` de ban demo nhanh tren Wokwi.

Khi het han, state chuyen sang `RETEST_REQUIRED`. He thong van giu servo mo khoa, bat LED xanh + vang va beep ngan de nhac. Nhan `TEST` se vao `RETEST_SAMPLING` trong khi xe van o trang thai running. PASS thi quay ve `RUNNING`; FAIL thi chuyen `FAIL_LOCKED`; neu het grace window ma khong test thi vao `ERROR_LOCKED` voi fault `RETEST_TIMEOUT`.

Checklist log can lay them cho bao cao:
- `STATE: Transition -> RETEST_REQUIRED`
- `STATE: Transition -> RETEST_SAMPLING`
- `AI_JSON` co `retestRequired=true`
- `METRIC: retest_due_to_test_ms=... | retest_to_result_ms=...`
- retest PASS: `sample_result` success va quay lai `RUNNING`
- retest FAIL: `sample_result` warning va chuyen `FAIL_LOCKED`
- retest timeout: `FAULT: RETEST_TIMEOUT`

## 13. Troubleshooting nhanh

### Wokwi không chạy

Kiểm tra:

- đã cài `Wokwi for VS Code`
- đã build ra `firmware.bin` và `firmware.elf`
- `wokwi.toml` còn trỏ đúng env `esp32dev`
- license Wokwi còn hiệu lực

### Dashboard không có dữ liệu

Kiểm tra:

- đang dùng `ESP32 thật`, không phải cửa sổ Wokwi
- baud rate là `115200`
- chọn đúng `COM` port
- firmware đã nạp bản mới nhất

### Nút bấm không ăn

Kiểm tra:

- `TEST -> GPIO14`
- `START -> GPIO16`
- module nút là `active HIGH` hay `active LOW`
- wiring trong Wokwi đang mô phỏng `active-HIGH` bằng pulldown 10k

### Muốn chỉnh threshold hoặc thời gian demo

Sửa trong `src/config.h`, sau đó build lại bằng `pio run`.

Lưu ý:

- các giá trị demo hiện tại phục vụ Wokwi, giúp log dễ nhìn và dễ trình bày
- nếu chuyển sang `MQ3` thật thì phải calibration lại threshold, warm-up và cách diễn giải giá trị ADC
