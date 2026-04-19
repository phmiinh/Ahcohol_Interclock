# Roadmap Vận Hành Dự Án

Tài liệu này là hướng dẫn chạy và nối các phần của project sau refactor. `README.md` trả lời câu hỏi dự án là gì; file này tập trung vào cách build, cách demo và cách nối firmware với dashboard.

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

## 6. Demo 3 flow chính

### Flow 1: Boot / Preheat

1. Start simulator
2. Quan sát `OLED` hiển thị warm-up
3. Quan sát `LED vàng` sáng
4. Chờ hết `PREHEAT_MS` hiện tại
5. Hệ thống tự chuyển sang `STANDBY_LOCKED`

### Flow 2: PASS rồi START

1. Đưa `potentiometer` xuống mức thấp hơn threshold
2. Nhấn `TEST`
3. Quan sát:
   - OLED vào `SAMPLING`
   - Serial log in thời gian lấy mẫu
   - Kết quả trung bình nhỏ hơn threshold
4. Hệ thống sang `PASS_READY`
5. Nhấn `START`
6. Servo mở khóa, state sang `RUNNING`

### Flow 3: FAIL rồi không cho START

1. Đưa `potentiometer` lên cao hơn threshold
2. Nhấn `TEST`
3. Quan sát:
   - OLED vào `SAMPLING`
   - Serial log average ADC, threshold, kết quả
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
- Phần cứng nút hiện mặc định là module `3 chân`, `active HIGH`
- Firmware bật bias nội cho input button để tránh idle bị float: `pulldown` khi `active HIGH`, `pullup` khi `active LOW`
- Nếu module của bạn xuất `LOW` khi nhấn, đổi `config::buttons::kActiveHigh` trong `src/config.h`
- Nếu dùng `MQ3` thật và module cho analog `0-5V`, phải chia áp trước khi đưa vào `GPIO34`
- Servo nên dùng nguồn đủ dòng và chung `GND` với ESP32

## 10. Fault handling hiện tại

Sau refactor, hệ thống có fault handling tối thiểu nhưng rõ ràng:

- `OLED init fail` -> vào `ERROR_LOCKED`, giữ safe state, log lỗi qua Serial
- `ADC read invalid` -> vào `ERROR_LOCKED`, khóa hệ thống
- Safe state luôn là:
  - servo khóa
  - không cho START
  - telemetry vẫn giữ được nếu Serial còn hoạt động

## 11. Checklist test trước khi demo

- `pio run` build thành công
- Wokwi local start được từ `diagram.json`
- `PREHEAT -> STANDBY_LOCKED` chuyển đúng
- Nhánh `PASS -> START -> RUNNING` chạy đúng
- Nhánh `FAIL -> START không có hiệu lực` chạy đúng
- Serial log có:
  - thời gian lấy mẫu
  - average ADC
  - threshold
  - kết quả PASS/FAIL
- Dashboard vẫn nhận được `AI_JSON` nếu test với board thật hoặc mock mode

## 12. Troubleshooting nhanh

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

### Muốn chỉnh threshold hoặc thời gian demo

Sửa trong `src/config.h`, sau đó build lại bằng `pio run`.
