# Roadmap Triển Khai Và Kiểm Thử

## 1. Mục Tiêu Tài Liệu

Tài liệu này mô tả quy trình thực hiện dự án Alcohol Interlock System theo góc nhìn kỹ thuật: bài toán, yêu cầu, thiết kế, triển khai, kiểm thử và các lưu ý khi chuyển từ Wokwi sang phần cứng thật.

`README.md` dùng để nắm tổng quan dự án. File này dùng để hiểu cách dự án được xây dựng và cách kiểm chứng từng phần.

## 2. Bài Toán Và Use Case

Bài toán đặt ra: trước khi cho phép mở khóa mô hình khởi động xe, hệ thống yêu cầu người dùng thực hiện kiểm tra nồng độ cồn. Nếu kết quả an toàn, hệ thống cho phép START. Nếu kết quả vượt ngưỡng, hệ thống khóa và cảnh báo. Sau khi xe đã ở trạng thái chạy, hệ thống tiếp tục yêu cầu kiểm tra lại theo chu kỳ để mô phỏng yêu cầu retest của alcohol interlock thực tế.

Các use case chính:

| Mã | Use case | Kết quả mong đợi |
|---|---|---|
| UC01 | Khởi động và preheat | Hệ thống vào `STANDBY_LOCKED` sau preheat |
| UC02 | Nhấn TEST để kiểm tra | Hệ thống lấy mẫu ADC và trả PASS/FAIL |
| UC03 | PASS rồi START | Servo mở, state vào `RUNNING` |
| UC04 | FAIL rồi không cho START | Servo giữ khóa, buzzer cảnh báo |
| UC05 | Retest khi đang RUNNING | Đến hạn thì yêu cầu TEST lại |
| UC06 | Retest timeout | Hệ thống vào `ERROR_LOCKED`, cho phép nhấn TEST để kiểm tra lại từ trạng thái khóa |
| UC07 | Lỗi cảm biến/ngoại vi | Hệ thống vào safe state |

## 3. Yêu Cầu Chức Năng

| Mã | Yêu cầu | Hiện trạng |
|---|---|---|
| FR01 | Khởi tạo GPIO, ADC, OLED, servo, buzzer | Đã có trong `IoDevices::begin()` |
| FR02 | Preheat trước khi đo | `PREHEAT` 10 giây demo |
| FR03 | Nhận TEST bằng interrupt/debounce | Đã có |
| FR04 | Lấy nhiều mẫu ADC | 20 mẫu trong khoảng 2 giây |
| FR05 | Tính trung bình và so threshold | `sampledAdc < kAlcoholAdc` là PASS |
| FR06 | Chỉ cho START sau PASS | Chỉ state `PASS_READY` mới nhận START |
| FR07 | Điều khiển servo khóa/mở | `GPIO15`, góc 0/90 |
| FR08 | OLED/LED/buzzer phản ánh trạng thái | Đã có |
| FR09 | Retest định kỳ khi RUNNING | 60 giây demo, 30 phút production |
| FR10 | Fault handling | OLED init fail, sensor timeout, retest timeout; riêng retest timeout có đường phục hồi bằng TEST |
| FR11 | Log phục vụ kiểm thử | Serial log gọn |

## 4. Yêu Cầu Phi Chức Năng

| Nhóm | Yêu cầu | Phân tích sơ bộ |
|---|---|---|
| An toàn | Mặc định khóa, lỗi thì khóa | Servo luôn về lock khi fault |
| Thời gian đáp ứng | Nút bấm phản hồi nhanh | Debounce 60 ms, interrupt flag |
| Ổn định tín hiệu | Không quyết định từ một mẫu | Trung bình 20 mẫu, có stddev |
| Dễ demo | Timing không quá dài | Sampling 2 giây, retest demo 60 giây |
| Dễ bảo trì | Code chia module | `config`, `io_devices`, `state_machine`, `telemetry`, `ui_oled` |
| Dễ kiểm thử | Có log và metric | `test_to_result_ms`, `start_to_unlock_ms`, retest metrics |
| Tài nguyên | Phù hợp ESP32 | RAM/Flash dưới giới hạn board |

## 5. Kiến Trúc Hệ Thống

Luồng tổng thể:

```text
Người dùng
  -> TEST/START
  -> ESP32
  -> FSM + ADC sampling
  -> OLED/LED/Buzzer/Servo
```

Các tầng phần mềm:

```text
main.cpp
  -> AlcoholInterlockController
    -> IoDevices
    -> OledUi
    -> Telemetry
    -> Config/AppTypes
```

Lý do chia module:

- `config.h`: gom toàn bộ pin, threshold, timing và feature flag.
- `io_devices.*`: cô lập thao tác phần cứng.
- `state_machine.*`: giữ business logic tập trung.
- `ui_oled.*`: render màn hình độc lập với FSM.
- `telemetry.*`: log và dashboard protocol không làm rối logic chính.

## 6. Thiết Kế Phần Cứng

Linh kiện dùng trong demo:

- ESP32 DevKit.
- OLED SSD1306 I2C.
- Servo SG90 hoặc tương đương.
- Buzzer.
- LED đỏ, xanh, vàng.
- Nút TEST và START.
- Potentiometer trong Wokwi để giả lập MQ3.

Pin mapping:

| Chức năng | GPIO |
|---|---:|
| ADC/MQ3 giả lập | `34` |
| Servo | `15` |
| Buzzer | `13` |
| LED đỏ | `25` |
| LED xanh | `26` |
| LED vàng | `27` |
| TEST | `14` |
| START | `16` |
| OLED SDA | `21` |
| OLED SCL | `22` |

Lưu ý quan trọng:

- START trên Wokwi DevKit được nối vào `RX2`, tương ứng GPIO16.
- Firmware mặc định active-LOW cho TEST/START.
- Nút active-LOW cần pullup.
- Servo cần nguồn đủ dòng và chung GND với ESP32.
- MQ3 thật cần hiệu chuẩn, không dùng threshold demo làm giá trị cuối.

## 7. Thiết Kế Phần Mềm

FSM hiện tại:

```text
PREHEAT
STANDBY_LOCKED
SAMPLING
PASS_READY
FAIL_LOCKED
RUNNING
RETEST_REQUIRED
RETEST_SAMPLING
ERROR_LOCKED
```

Các nguyên tắc chính:

- Không dùng `delay()` cho sampling hoặc buzzer.
- Dùng `millis()` để lập lịch.
- Mỗi vòng update chỉ lấy tối đa một mẫu ADC.
- ISR của nút chỉ set flag, không xử lý nghiệp vụ trong interrupt.
- Mọi lỗi nghiêm trọng đều đi về safe state.

## 8. Quy Trình Build Và Mô Phỏng

### 8.1. Build Firmware

```powershell
pio run
```

Kết quả cần có:

```text
.pio/build/esp32dev/firmware.bin
.pio/build/esp32dev/firmware.elf
```

### 8.2. Chạy Wokwi

1. Build firmware.
2. Mở `diagram.json`.
3. Chạy `Wokwi: Start Simulator`.
4. Quan sát OLED, servo, LED, buzzer và Serial Monitor.

### 8.3. Chạy Dashboard

```powershell
npm install
npm run start
```

Hoặc chạy mock:

```powershell
npm run mock
```

Dashboard chỉ có ý nghĩa khi firmware bật `kEnableDashboardProtocol = true`.

## 9. Quy Trình Kiểm Thử

### 9.1. Test Boot/Preheat

Mục tiêu:

- Kiểm tra hệ thống khởi động đúng.
- Servo về lock.
- OLED hiển thị preheat.
- Sau 10 giây vào `STANDBY_LOCKED`.

Log mong đợi:

```text
STATE: Transition -> PREHEAT
STATE: Transition -> STANDBY_LOCKED
```

### 9.2. Test PASS Rồi START

Thao tác:

1. Đưa ADC xuống dưới threshold.
2. Nhấn TEST.
3. Chờ sampling khoảng 2 giây.
4. Quan sát `PASS_READY`.
5. Nhấn START.

Log mong đợi:

```text
RESULT: ... result=PASS
STATE: Transition -> PASS_READY
ACTION: START accepted. Vehicle unlocked
STATE: Transition -> RUNNING
```

### 9.3. Test FAIL Rồi Không Cho START

Thao tác:

1. Đưa ADC lên trên threshold.
2. Nhấn TEST.
3. Chờ sampling.
4. Nhấn START để xác nhận không mở khóa.

Kết quả:

- State giữ ở `FAIL_LOCKED`.
- Servo không mở.
- Buzzer cảnh báo.

### 9.4. Test Retest Định Kỳ

Thao tác:

1. Chạy flow PASS rồi START.
2. Hệ thống vào `RUNNING`.
3. Chờ 60 giây trong demo.
4. Quan sát state `RETEST_REQUIRED`.
5. Buzzer phải kêu lặp liên tục theo pattern FAIL để nhắc retest.
6. Nhấn TEST.

Kết quả:

- Retest PASS: quay lại `RUNNING`.
- Retest FAIL: vào `FAIL_LOCKED`.
- Không nhấn TEST trong grace window: vào `ERROR_LOCKED`.
- Sau `RETEST_TIMEOUT`: START bị bỏ qua, TEST chạy sampling lại từ trạng thái khóa.

### 9.5. Test START Trên Phần Cứng Thật

Với cấu hình active-LOW:

| Trạng thái | Điện áp mong đợi tại D16/GPIO16 |
|---|---:|
| Không nhấn START | khoảng `3.3V` |
| Nhấn START | khoảng `0V` |

Nếu đo ngược lại, đổi `kActiveHigh = true`.

## 10. Phân Tích Sơ Bộ Về Timing

| Tác vụ | Timing hiện tại | Nhận xét |
|---|---:|---|
| Preheat demo | 10 giây | Đủ nhìn rõ khi demo |
| Sampling | 2 giây | Cân bằng giữa tốc độ và ổn định mẫu |
| Debounce nút | 60 ms | Phù hợp nút cơ |
| Guard START sau RUNNING | 800 ms | Chống một lần bấm bị tính hai lần |
| Retest demo | 60 giây | Dễ quan sát mà không quá gấp |
| Retest production | 30 phút | Gần yêu cầu thực tế hơn |
| Retest grace demo | 15 giây | Đủ để thấy timeout |

## 11. Phân Tích Sơ Bộ Về Rủi Ro

### 11.1. Nút Bấm

Rủi ro:

- Sai polarity active-HIGH/active-LOW.
- Dây input bị float.
- Module nút không chung GND với ESP32.

Giải pháp:

- Đo điện áp thực tế ở chân GPIO.
- Giữ `kActiveHigh = false` nếu idle HIGH, nhấn LOW.
- Đổi `kActiveHigh = true` nếu idle LOW, nhấn HIGH.

### 11.2. Servo

Rủi ro:

- Servo kéo dòng làm ESP32 reset.
- Servo không có GND chung.
- Cắm nhầm chân signal.

Giải pháp:

- Dùng nguồn 5V riêng đủ dòng.
- Nối chung GND.
- Signal servo đúng GPIO15.

### 11.3. Cảm Biến MQ3 Thật

Rủi ro:

- Warm-up thật lâu hơn demo.
- Threshold demo không đúng thực tế.
- Analog output vượt mức an toàn ESP32.

Giải pháp:

- Hiệu chuẩn lại `kAlcoholAdc`.
- Đo `sampleStdDev`.
- Bảo đảm điện áp vào GPIO34 trong miền an toàn.

## 12. Tiêu Chí Hoàn Thành Demo

Repo được coi là sẵn sàng demo khi:

- `pio run` build thành công.
- Wokwi chạy được từ `diagram.json`.
- PASS rồi START mở servo ổn định.
- FAIL không mở servo.
- Retest sau 60 giây có buzzer nhắc liên tục theo pattern FAIL.
- Retest PASS/FAIL hoạt động đúng.
- Không retest thì timeout vào `ERROR_LOCKED`, sau đó TEST có thể chạy lại kiểm tra.
- Serial log đủ gọn để copy vào báo cáo.

## 13. Việc Cần Làm Nếu Muốn Nâng Cấp

- Hiệu chuẩn MQ3 thật bằng dữ liệu đo thực nghiệm.
- Tách cấu hình demo và production rõ hơn bằng build flag.
- Bật dashboard protocol khi dùng ESP32 thật.
- Bổ sung test log thực tế vào báo cáo.
- Đóng gói prototype phần cứng với nguồn servo ổn định.
