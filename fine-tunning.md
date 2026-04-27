# Fine-tuning Sau Khi Bổ Sung Retest

## 1. Mục Tiêu

Tài liệu này ghi lại các lần tinh chỉnh quan trọng sau khi bổ sung cơ chế retest định kỳ cho Alcohol Interlock System. Nội dung tập trung vào lý do thay đổi, vấn đề đã gặp và trạng thái cuối cùng của firmware.

Phạm vi bắt đầu từ lúc thêm yêu cầu:

```text
Sau khi PASS và START, hệ thống đang RUNNING phải yêu cầu TEST lại theo chu kỳ.
```

Trạng thái hiện tại:

- Production retest: 30 phút một lần.
- Demo retest: 60 giây một lần.
- Sampling: 20 mẫu trong khoảng 2 giây.
- Nút TEST/START: active-LOW, interrupt flag + debounce.
- START sau PASS được bảo vệ để không tự chạy nếu tín hiệu START đang active sẵn.
- Buzzer phản hồi cho PASS, START và yêu cầu retest.

## 2. Thêm Cơ Chế Retest Định Kỳ

### Vấn đề cần giải quyết

Phiên bản đầu chỉ kiểm tra trước khi START. Điều này đủ cho demo cơ bản nhưng chưa phản ánh đúng ý tưởng alcohol interlock thực tế, vì người dùng có thể cần kiểm tra lại trong quá trình vận hành.

### Thay đổi đã thực hiện

Thêm các trạng thái phục vụ retest:

- `RETEST_REQUIRED`
- `RETEST_SAMPLING`

Thêm fault code liên quan:

- `RETEST_TIMEOUT`, được hiển thị như lỗi và đưa hệ thống về `ERROR_LOCKED`

Luồng mới:

```text
RUNNING -> RETEST_REQUIRED -> RETEST_SAMPLING
```

Kết quả:

```text
RETEST_SAMPLING -> RUNNING      nếu PASS
RETEST_SAMPLING -> FAIL_LOCKED  nếu FAIL
RETEST_REQUIRED -> ERROR_LOCKED nếu timeout
ERROR_LOCKED -> SAMPLING        nếu fault là RETEST_TIMEOUT và người dùng nhấn TEST
```

### Trạng thái hiện tại

- Retest production: `30 phút`.
- Retest demo: `60 giây`.
- Grace demo: `15 giây`.
- Khi yêu cầu retest, servo vẫn mở, LED xanh + vàng bật, buzzer kêu nhắc.
- Nếu quá grace window, hệ thống khóa vào `ERROR_LOCKED`; với riêng `RETEST_TIMEOUT`, người dùng có thể nhấn TEST để kiểm tra lại từ trạng thái khóa.

## 3. Mở Rộng Snapshot Và Telemetry

### Thay đổi

Snapshot được bổ sung các trường retest:

- `retestRequired`
- `retestIntervalMs`
- `retestRemainingMs`
- `retestOverdueMs`
- `retestGraceRemainingMs`
- `retestDueToTestMs`
- `retestToResultMs`
- `runningSessionMs`
- `retestCycleCount`

### Mục đích

- Có dữ liệu để viết báo cáo.
- Có thể hiển thị countdown retest trên OLED/dashboard.
- Đo được phản ứng của người dùng sau khi hệ thống yêu cầu retest.

## 4. Giảm Nhiễu Serial Log

### Vấn đề

Sau khi thêm telemetry, Serial Monitor bị spam bởi:

- Live ADC debug.
- `AI_JSON` định kỳ.
- Log từng mẫu sampling.

Điều này làm khó theo dõi demo.

### Thay đổi

Tắt mặc định:

```cpp
kEnableSensorDebug = false
kEnableDashboardProtocol = false
kEnableSampleProgressLog = false
```

### Kết quả

Serial Monitor hiện chủ yếu còn:

- Boot log.
- State transition.
- Action như TEST/START.
- Sampling result.
- Metric quan trọng.
- Warning/fault.

## 5. Điều Chỉnh SENSOR_TIMEOUT Cho Wokwi

### Vấn đề

Trong Wokwi, người dùng thường vặn potentiometer sát 0 hoặc 4095. Firmware cũ coi toàn bộ mẫu sát rail là lỗi cảm biến cứng và vào `SENSOR_TIMEOUT`.

### Thay đổi

Thêm:

```cpp
kFaultOnAllRailSamples = !kDemoMode
```

### Kết quả

- Demo mode: rail ADC chỉ log warning, vẫn cho PASS/FAIL.
- Production mode: có thể giữ hành vi fail-safe nghiêm ngặt.

## 6. Sửa Nút START Trong Wokwi

### Vấn đề

Sau PASS, nhấn START không có log, servo không chạy.

### Nguyên nhân

Firmware dùng GPIO16, nhưng trên `wokwi-esp32-devkit-v1`, chân này được ghi là `RX2`. Wiring cũ nối START vào `D16`, gây khả năng tín hiệu không vào đúng chân.

### Thay đổi

Trong `diagram.json`, START được nối vào:

```text
esp:RX2
```

Firmware vẫn giữ:

```cpp
kButtonStart = 16
```

## 7. Chuyển TEST/START Sang Active-LOW

### Vấn đề

Khi lắp phần cứng thật, nút TEST chỉ hoạt động nếu `kActiveHigh = false`.

### Phân tích

Module nút thật đang hoạt động theo kiểu active-LOW:

- Không nhấn: HIGH.
- Nhấn: LOW.

Nếu firmware để active-HIGH, hệ thống sẽ đọc sai mức nhấn.

### Thay đổi

```cpp
kActiveHigh = false
```

Wokwi cũng được sửa sang mô hình:

- Pullup 10k.
- Nhấn nút kéo tín hiệu xuống GND.

## 8. Thêm Interrupt Cho START

### Vấn đề

TEST đã dùng interrupt, START ban đầu vẫn phụ thuộc nhiều vào polling. Click ngắn hoặc nhiễu trên Wokwi/phần cứng có thể bị bỏ lỡ.

### Thay đổi

Thêm:

- `onStartButtonIsr()`
- `startIrqFlag_`
- `clearButtonEvent()`

Nguyên tắc vẫn giữ:

- ISR chỉ set flag.
- Debounce và nghiệp vụ chạy trong vòng `loop()`.

## 9. Bảo Vệ PASS_READY

### Vấn đề

Trên phần cứng thật có tình huống sampling PASS xong hệ thống nhảy thẳng sang `RUNNING`, không có thời gian chờ người dùng nhấn START.

### Nguyên nhân hợp lý

START có thể đang active hoặc còn event pending ngay khi vừa chuyển sang `PASS_READY`.

### Thay đổi

Khi vào `PASS_READY`:

- Clear event START cũ.
- Kiểm tra START đang nhả hay đang active.
- Chỉ chấp nhận START nếu đã thấy START nhả ra sau khi vào `PASS_READY`.

Biến bảo vệ:

```cpp
startReleasedAfterPassReady_
```

### Kết quả

- PASS chỉ cấp quyền START.
- Firmware không tự mở khóa nếu START đang active sẵn.
- Người dùng phải nhả START rồi nhấn lại để vào `RUNNING`.

## 10. Chống START Bị Tính Hai Lần

### Vấn đề

Sau khi nhấn START, servo có thể gạt mở rồi lập tức quay về khóa.

### Nguyên nhân

Cùng một lần bấm START có thể bị nhận thêm một event sau khi hệ thống đã vào `RUNNING`. Trong `RUNNING`, START có chức năng phụ là khóa lại về `STANDBY_LOCKED`.

### Thay đổi

Thêm guard:

```cpp
kRunningStartRelockGuardMs = 800
```

### Kết quả

- START ở `PASS_READY` mở khóa.
- Trong 800 ms đầu sau khi vào `RUNNING`, START không thể relock.
- Muốn khóa lại, người dùng phải nhấn START lần nữa sau guard window.

## 11. Tinh Chỉnh Sampling

### Diễn biến

Sampling từng được tăng lên 10 giây để thử giống quá trình đo thật hơn. Sau đó giảm lại vì quá dài cho demo và không cần thiết với prototype hiện tại.

### Trạng thái hiện tại

```cpp
kSampleCount = 20
kSampleTotalMs = 2000
```

Kết quả:

- 20 mẫu trong khoảng 2 giây.
- Mỗi mẫu cách nhau khoảng 105 ms.
- Demo nhanh hơn nhưng vẫn đủ dữ liệu để tính trung bình và độ lệch chuẩn.

## 12. Buzzer Sau Các Lần Tinh Chỉnh

### PASS

Khi test PASS, buzzer kêu một tiếng ngắn.

### START

Khi START hợp lệ từ `PASS_READY`, buzzer kêu một tiếng ngắn tương tự PASS.

### FAIL

Khi FAIL, buzzer cảnh báo theo chu kỳ.

### Retest

Khi đến hạn retest, state vào `RETEST_REQUIRED` và buzzer kêu nhắc theo chu kỳ. Pha buzzer được tính từ thời điểm vào `RETEST_REQUIRED`, nên sẽ có tiếng nhắc ngay khi state này bắt đầu.

## 13. Trạng Thái Cuối Cùng Của Demo

Thông số hiện tại:

| Nhóm | Giá trị |
|---|---:|
| Preheat demo | 10 giây |
| Sampling | 2 giây |
| Retest demo | 60 giây |
| Retest production | 30 phút |
| Retest grace demo | 15 giây |
| Button polarity | active-LOW |
| START GPIO | GPIO16/RX2 |
| Servo GPIO | GPIO15 |
| Threshold demo | ADC 2000 |

Luồng demo chuẩn:

```text
PREHEAT
-> STANDBY_LOCKED
-> TEST
-> SAMPLING
-> PASS_READY
-> START
-> RUNNING
-> sau 60 giây
-> RETEST_REQUIRED
-> TEST
-> RETEST_SAMPLING
-> RUNNING hoặc FAIL_LOCKED
```

## 14. Bài Học Kỹ Thuật

Các lỗi đã gặp không nằm ở thuật toán ADC mà chủ yếu nằm ở ranh giới giữa firmware và phần cứng:

- Tên chân trên Wokwi có thể khác tên GPIO trong code.
- Nút active-HIGH và active-LOW nếu cấu hình sai sẽ gây hành vi rất khó đoán.
- Event interrupt cũ có thể ảnh hưởng state mới nếu không clear đúng thời điểm.
- Servo cần nguồn riêng đủ dòng; nếu không, ESP32 có thể reset khi servo chạy.
- Log quá nhiều làm việc debug khó hơn, đặc biệt trong demo.

## 15. Trạng Thái Sẵn Sàng

Firmware hiện đã phù hợp để demo:

- Có retest định kỳ 1 phút trong demo.
- Có buzzer nhắc retest.
- Có tiếng beep khi PASS và khi START hợp lệ.
- Có bảo vệ START sau PASS.
- Có guard chống START double event.
- Có sampling 2 giây.
- Có tài liệu README và roadmap đã đồng bộ với code hiện tại.
