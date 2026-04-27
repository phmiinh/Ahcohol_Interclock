# Fine Tunning Review

## 1. Phạm Vi Review

Tài liệu này tổng hợp các thay đổi đã thực hiện từ thời điểm bổ sung cơ chế retest định kỳ khi hệ thống đang chạy đến trạng thái hiện tại của firmware.

Mốc bắt đầu:

- Thêm yêu cầu: sau khi `PASS -> START -> RUNNING`, hệ thống phải yêu cầu kiểm tra lại theo chu kỳ.
- Mục tiêu production: retest mỗi `30 phút`.
- Mục tiêu demo Wokwi: rút xuống `30 giây` để có thể quan sát trong buổi demo.

Mốc hiện tại:

- Firmware đã có luồng retest.
- Serial log đã được giảm nhiễu.
- START đã đọc ổn định hơn trên Wokwi và phần cứng thật.
- Sampling test đã được chốt lại ở khoảng `2 giây` cho demo và phần cứng thật.
- Luồng `PASS_READY` đã được bảo vệ để không tự nhảy sang `RUNNING` nếu START đang bị giữ hoặc bị active sẵn.

## 2. Thay Đổi Chính Theo Nhóm Chức Năng

### 2.1. Bổ Sung Periodic Retest Khi Đang RUNNING

Các trạng thái mới đã được thêm vào FSM:

- `RETEST_REQUIRED`
- `RETEST_SAMPLING`
- `ERROR_LOCKED` với fault `RETEST_TIMEOUT`

Luồng nghiệp vụ hiện tại:

1. Người dùng TEST đạt PASS.
2. Hệ thống vào `PASS_READY`.
3. Người dùng nhấn START.
4. Hệ thống mở servo và vào `RUNNING`.
5. Firmware bắt đầu đếm timer retest.
6. Khi đến hạn, hệ thống chuyển sang `RETEST_REQUIRED`.
7. Servo vẫn mở, LED xanh + vàng bật, buzzer nhắc.
8. Người dùng nhấn TEST để vào `RETEST_SAMPLING`.
9. Retest PASS thì quay lại `RUNNING` và lập lịch chu kỳ mới.
10. Retest FAIL thì chuyển `FAIL_LOCKED`.
11. Nếu quá grace window mà không retest thì vào `ERROR_LOCKED` với fault `RETEST_TIMEOUT`.

Thông số hiện tại:

- Production interval: `30 * 60 * 1000 ms`.
- Demo interval: `30 * 1000 ms`.
- Production grace: `5 phút`.
- Demo grace: `15 giây`.

Ý nghĩa kỹ thuật:

- Tính năng này làm hệ thống gần với alcohol interlock thực tế hơn, vì không chỉ kiểm tra trước khi khởi động mà còn có kiểm tra lại khi đang vận hành.
- Servo vẫn mở trong `RETEST_REQUIRED` và `RETEST_SAMPLING`, tránh hành vi khóa đột ngột ngay khi vừa đến hạn retest.

### 2.2. Mở Rộng Telemetry Cho Retest

Telemetry `AI_JSON` và snapshot đã có thêm các trường:

- `retestRequired`
- `retestIntervalMs`
- `retestRemainingMs`
- `retestOverdueMs`
- `retestGraceRemainingMs`
- `retestDueToTestMs`
- `retestToResultMs`
- `runningSessionMs`
- `retestCycleCount`

Các metric này phục vụ báo cáo và dashboard:

- `retestRemainingMs`: còn bao lâu tới lần retest tiếp theo.
- `retestDueToTestMs`: người dùng mất bao lâu từ lúc được yêu cầu retest đến lúc nhấn TEST.
- `retestToResultMs`: thời gian từ lúc nhấn TEST trong retest đến khi có PASS/FAIL.
- `retestCycleCount`: số chu kỳ retest đã phát sinh trong một phiên RUNNING.

### 2.3. Giảm Spam Log Serial

Vấn đề gặp phải:

- Serial Monitor bị tràn bởi live ADC debug và `AI_JSON` định kỳ.
- Khi demo Wokwi, log quá nhiều làm khó theo dõi state machine.

Điều chỉnh đã thực hiện:

- Tắt mặc định `kEnableSensorDebug`.
- Tắt mặc định `kEnableDashboardProtocol`.
- Thêm `kEnableSampleProgressLog` và tắt mặc định.

Hành vi log hiện tại:

- Vẫn log boot.
- Vẫn log state transition.
- Vẫn log action như TEST/START.
- Vẫn log kết quả sampling.
- Vẫn log warning/fault quan trọng.
- Không còn bắn live ADC và `AI_JSON` liên tục trong Wokwi Serial Monitor.

Lưu ý:

- Nếu cần dashboard realtime với ESP32 thật, bật lại `kEnableDashboardProtocol = true`.
- Nếu cần debug từng mẫu ADC, bật lại `kEnableSampleProgressLog = true`.

### 2.4. Điều Chỉnh SENSOR_TIMEOUT Cho Demo Wokwi

Vấn đề gặp phải:

- Khi vặn potentiometer sát `0` hoặc `4095`, toàn bộ sampling có thể nằm sát rail ADC.
- Firmware cũ coi đây là lỗi cứng `SENSOR_TIMEOUT`, dẫn tới `ERROR_LOCKED`.
- Trong Wokwi, vặn hết cỡ là thao tác demo bình thường, không nhất thiết là lỗi wiring.

Điều chỉnh đã thực hiện:

- Thêm cấu hình `kFaultOnAllRailSamples`.
- Trong `kDemoMode = true`, rail-saturated sampling chỉ log warning và vẫn cho PASS/FAIL.
- Khi production, có thể bật strict fail-safe để rail-saturated sampling vào fault.

Ý nghĩa:

- Demo dễ chạy hơn.
- Vẫn giữ được hướng fail-safe cho phần cứng thật.

### 2.5. Sửa Nút START Không Ăn Trên Wokwi

Vấn đề gặp phải:

- Sau khi PASS, nhấn START không có log.
- Không có dòng `ACTION: START accepted. Vehicle unlocked`.
- Servo không chạy vì firmware không nhận START.

Nguyên nhân chính:

- Firmware dùng GPIO16.
- Trên Wokwi board `esp32-devkit-v1`, chân GPIO16 được label là `RX2`, không phải `D16`.
- `diagram.json` trước đó nối START vào `esp:D16`, nên tín hiệu có khả năng không vào đúng chân MCU.

Điều chỉnh đã thực hiện:

- Sửa wiring START trong `diagram.json` sang `esp:RX2`.
- Giữ firmware ở `kButtonStart = 16`.

Kết quả mong đợi:

- Sau `PASS_READY`, nhấn START sẽ sinh log:

```text
ACTION: START accepted. Vehicle unlocked
STATE: Transition -> RUNNING | locked=false ...
```

### 2.6. Thêm Interrupt Cho START

Ban đầu:

- TEST có interrupt.
- START chủ yếu dựa vào polling/debounce.

Vấn đề:

- Click ngắn trong Wokwi hoặc nhiễu thao tác có thể bị bỏ lỡ.

Điều chỉnh:

- Thêm `onStartButtonIsr()`.
- Thêm `startIrqFlag_`.
- START và TEST cùng dùng mô hình interrupt flag + software debounce.

Ý nghĩa:

- START nhạy và ổn định hơn.
- ISR vẫn chỉ set flag, logic nghiệp vụ vẫn xử lý ở `loop()`, đúng hướng an toàn cho embedded.

### 2.7. Chống START Bị Tính Hai Lần Sau Khi Unlock

Vấn đề gặp phải:

- Nhấn START sau PASS làm servo/barie chỉ gạt một cái rồi quay lại.
- Nguyên nhân hợp lý: cùng một lần bấm START có thể bị nhận thêm một edge sau khi đã vào `RUNNING`.
- Firmware khi ở `RUNNING` có flow phụ: bấm START lần nữa để quay về `STANDBY_LOCKED`.

Điều chỉnh:

- Thêm `kRunningStartRelockGuardMs = 800`.
- Trong 800 ms đầu sau khi vào `RUNNING`, START không được dùng để relock.

Hành vi hiện tại:

- START tại `PASS_READY` mở khóa ngay.
- Servo giữ ở góc mở.
- Muốn khóa lại, người dùng phải bấm START lần nữa sau guard window.

### 2.8. Chốt Thời Gian Sampling 2 Giây

Yêu cầu mới:

- Sau khi thử `10 giây`, thời gian sampling được giảm xuống `2 giây` vì 10 giây làm demo chậm và không cần thiết cho prototype hiện tại.

Điều chỉnh:

- `kSampleTotalMs` hiện được chốt là `2000 ms`.
- `kSampleCount` giữ là `20`.
- `kSampleIntervalMs` tự tính lại theo công thức:

```cpp
kSampleIntervalMs = kSampleTotalMs / (kSampleCount - 1)
```

Kết quả:

- 20 mẫu được lấy trong khoảng 2 giây.
- Mỗi mẫu cách nhau khoảng `105 ms`.
- Log `Sampling finished in ... ms` sẽ xấp xỉ 2 giây, có thể lệch nhẹ do loop/UI/Serial overhead.

Ý nghĩa:

- Demo nhanh hơn nhưng vẫn có đủ nhiều mẫu để lọc nhiễu cơ bản.
- Không kéo dài flow demo quá mức.
- Metric `test_to_result_ms` trong báo cáo phải cập nhật theo mốc mới, khoảng 2 giây.

### 2.9. Bảo Vệ PASS_READY Không Tự Nhảy RUNNING

Vấn đề phần cứng thật:

- Sau khi test PASS, hệ thống có thể nhảy thẳng sang màn hình `RUNNING`.
- Người dùng gần như không có cơ hội nhấn START.

Phân tích:

- Trong source, `SAMPLING` sau PASS chỉ chuyển sang `PASS_READY`, không tự chuyển sang `RUNNING`.
- Nếu nhảy sang `RUNNING`, nghĩa là firmware đã nhận `startPressed = true` ngay sau khi vào `PASS_READY`.
- Nguyên nhân có thể là:
  - START đang bị giữ từ trước.
  - START bị float.
  - Module nút START active-LOW nhưng firmware cấu hình active-HIGH.
  - Dây hoặc bias phần cứng làm GPIO16 luôn active.
  - Interrupt START còn pending khi chuyển state.

Điều chỉnh đã thực hiện:

- Thêm hàm `buttonActive(ButtonId button)` để đọc trạng thái hiện tại của nút.
- Thêm cờ `startReleasedAfterPassReady_`.
- Khi vừa vào `PASS_READY`, firmware kiểm tra START có đang nhả không.
- `PASS_READY` chỉ chấp nhận START nếu đã thấy nút START nhả ra ít nhất một lần sau khi vào state này.

Hành vi mong muốn:

- Nếu START đang active sẵn lúc vừa PASS, hệ thống vẫn đứng ở `PASS_READY`.
- Người dùng phải nhả START rồi bấm lại để vào `RUNNING`.
- Điều này làm flow an toàn hơn cho phần cứng thật.

## 3. Trạng Thái FSM Hiện Tại

Luồng chính:

```text
PREHEAT
  -> STANDBY_LOCKED
  -> SAMPLING
  -> PASS_READY
  -> RUNNING
```

Luồng FAIL:

```text
SAMPLING
  -> FAIL_LOCKED
```

Luồng retest:

```text
RUNNING
  -> RETEST_REQUIRED
  -> RETEST_SAMPLING
  -> RUNNING      nếu PASS
  -> FAIL_LOCKED  nếu FAIL
  -> ERROR_LOCKED nếu timeout
```

Luồng lỗi:

```text
OLED_INIT_FAILED -> ERROR_LOCKED
SENSOR_TIMEOUT   -> ERROR_LOCKED nếu strict rail fault bật
RETEST_TIMEOUT   -> ERROR_LOCKED
```

## 4. Các Thông Số Cấu Hình Quan Trọng Hiện Tại

| Tham số | Giá trị hiện tại | Ý nghĩa |
|---|---:|---|
| `kDemoMode` | `true` | Dùng timing demo Wokwi |
| `kPreheatMs` | `10000 ms` | Preheat demo 10 giây |
| `kSampleCount` | `20` | Số mẫu mỗi lần test |
| `kSampleTotalMs` | `2000 ms` | Tổng thời gian sampling |
| `kSampleIntervalMs` | khoảng `105 ms` | Khoảng cách giữa 2 mẫu |
| `kRetestDemoMs` | `30000 ms` | Retest sau 30 giây trong demo |
| `kRetestProductionMs` | `1800000 ms` | Retest sau 30 phút trong production |
| `kRetestGraceMs` | `15000 ms` ở demo | Thời gian chờ người dùng retest |
| `kButtonDebounceMs` | `60 ms` | Debounce phần mềm |
| `kRunningStartRelockGuardMs` | `800 ms` | Chặn START bị tính hai lần sau unlock |
| `kEnableSensorDebug` | `false` | Không spam ADC log |
| `kEnableDashboardProtocol` | `false` | Không spam AI_JSON trong Wokwi |
| `kEnableSampleProgressLog` | `false` | Không log từng mẫu |

## 5. Điểm Cần Lưu Ý Khi Lắp Phần Cứng Thật

### 5.1. Polarity Của Nút

Firmware hiện giả định:

```cpp
kActiveHigh = true
```

Nghĩa là:

- Idle: LOW.
- Nhấn: HIGH.
- Nên có pulldown ổn định.

Nếu module nút thực tế của bạn là active-LOW:

- Idle: HIGH.
- Nhấn: LOW.

Khi đó phải đổi:

```cpp
kActiveHigh = false
```

Nếu không đổi, firmware có thể hiểu nhầm rằng START đang được nhấn ngay khi vừa PASS.

### 5.2. Bias Cho START Và TEST

Nếu dùng nút rời:

- Active-HIGH: cần pulldown.
- Active-LOW: cần pullup.

Firmware đang bật internal bias theo cấu hình, nhưng với dây dài hoặc module ngoài, vẫn nên có bias phần cứng rõ ràng.

### 5.3. Servo

Servo nên có nguồn riêng đủ dòng và chung GND với ESP32.

Triệu chứng nguồn servo yếu:

- Servo giật nhẹ rồi quay lại.
- ESP32 reset bất thường.
- OLED chớp hoặc mất hiển thị.
- Serial có boot log lại từ đầu.

### 5.4. MQ3 Thật Khác Potentiometer

Wokwi dùng potentiometer để giả lập ADC.

Với MQ3 thật:

- Cần warm-up dài hơn 10 giây.
- Cần hiệu chuẩn threshold.
- Cần bảo đảm analog output không vượt mức an toàn của GPIO34.
- Cần test nhiễu và độ ổn định bằng `sampleStdDev`.

## 6. Checklist Xác Minh Sau Các Thay Đổi

### 6.1. Wokwi

- Boot vào `PREHEAT`.
- Sau 10 giây vào `STANDBY_LOCKED`.
- Nhấn TEST, OLED vào `SAMPLING`.
- Sampling kéo dài khoảng 2 giây.
- PASS thì vào `PASS_READY`, OLED hiện yêu cầu START.
- Nhấn START, log có `START accepted`.
- Servo giữ ở góc mở, state là `RUNNING`.
- Sau 30 giây demo, state vào `RETEST_REQUIRED`.
- Nhấn TEST để retest.
- Retest PASS quay lại `RUNNING`.
- Retest FAIL vào `FAIL_LOCKED`.
- Không nhấn TEST trong grace window thì vào `ERROR_LOCKED` với `RETEST_TIMEOUT`.

### 6.2. Phần Cứng Thật

- Đo điện áp chân START khi không nhấn.
- Đo điện áp chân START khi nhấn.
- Xác nhận polarity khớp `kActiveHigh`.
- Xác nhận servo không làm ESP32 reset.
- Xác nhận sau PASS hệ thống đứng ở `PASS_READY`, không tự mở khóa.
- Chỉ sau khi nhấn START mới vào `RUNNING`.

## 7. Kết Luận Review

Các thay đổi từ lúc thêm retest 30 phút đến hiện tại đã đưa firmware từ một demo PASS/FAIL đơn giản thành một FSM thực tế hơn:

- Có retest định kỳ khi đang chạy.
- Có grace window và fault timeout.
- Có telemetry phục vụ báo cáo.
- Có log gọn hơn cho demo.
- Có xử lý riêng cho Wokwi và production đối với rail ADC.
- Có START interrupt và sửa wiring Wokwi đúng GPIO16/RX2.
- Có guard chống START bị tính hai lần.
- Có bảo vệ `PASS_READY` để không tự nhảy `RUNNING` khi START đang active sẵn.
- Có sampling window 2 giây, cân bằng giữa độ ổn định mẫu và tốc độ demo.

Rủi ro còn lại chủ yếu nằm ở phần cứng thật:

- Polarity module nút không khớp cấu hình.
- Bias input chưa đủ ổn định.
- Nguồn servo yếu.
- MQ3 chưa được hiệu chuẩn threshold và warm-up đúng thực tế.

Về logic firmware, luồng `PASS_READY -> START -> RUNNING` hiện đã được bảo vệ tốt hơn và phù hợp hơn với yêu cầu interlock: kết quả PASS chỉ cấp quyền START, không tự mở khóa.

