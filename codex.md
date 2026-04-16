
---

# Bản tóm tắt dự án để chuyển cho Codex

## 1. Mục tiêu dự án

Tôi đang làm một đồ án môn **Xây dựng hệ thống nhúng** với đề tài:

**Alcohol Interlock System**
(Hệ thống khóa liên động kiểm tra nồng độ cồn trước khi cho phép khởi động xe)

Mục tiêu của hệ thống:

* người dùng phải thực hiện kiểm tra nồng độ cồn trước khi khởi động
* nếu nồng độ cồn an toàn thì hệ thống cho phép khởi động
* nếu vượt ngưỡng thì hệ thống khóa, không cho khởi động và phát cảnh báo

Đây là mô hình học phần / prototype nhúng, không phải thiết bị đo thương mại chính xác.

---

## 2. Kiến trúc tổng thể

Hệ thống gồm các khối chính:

* **Khối xử lý trung tâm:** ESP32
* **Khối cảm biến đầu vào:** MQ3 trong phần cứng thật, nhưng khi mô phỏng Wokwi thì thay bằng **potentiometer** để giả lập tín hiệu analog
* **Khối chấp hành:** Servo SG90 để mô phỏng cơ cấu khóa/mở chốt
* **Khối giao tiếp và cảnh báo:** OLED SSD1306, buzzer, LED xanh/vàng/đỏ
* **Khối nhập liệu:** 2 nút bấm

  * nút TEST: đo nồng độ cồn
  * nút START: khởi động khi đã pass

Bản thiết kế ban đầu từng nhắc đến relay, nhưng phương án hiện tại đã **chốt dùng servo làm chấp hành chính** để dễ mô phỏng và dễ demo hơn. 

---

## 3. Luồng hoạt động mong muốn

Logic hệ thống được triển khai theo dạng **state machine**:

1. **PREHEAT**

   * hệ thống khởi động
   * làm nóng cảm biến
   * LED vàng sáng
   * servo ở trạng thái khóa
   * chưa cho phép START

2. **STANDBY_LOCKED**

   * chờ người dùng nhấn TEST
   * hệ thống vẫn đang khóa

3. **SAMPLING**

   * khi nhấn TEST, hệ thống đọc nhiều mẫu analog
   * lấy trung bình để giảm nhiễu
   * sau đó so sánh với ngưỡng

4. **PASS_READY**

   * nếu giá trị dưới ngưỡng
   * LED xanh sáng
   * buzzer không kêu
   * cho phép nhấn START

5. **FAIL_LOCKED**

   * nếu giá trị vượt ngưỡng
   * LED đỏ sáng
   * buzzer cảnh báo
   * servo giữ khóa
   * START không có hiệu lực

6. **RUNNING**

   * khi đang ở PASS_READY và người dùng nhấn START
   * servo quay sang góc mở khóa
   * hệ thống chuyển sang trạng thái vận hành

---

## 4. Pin mapping hiện tại

Pin mapping đang dùng:

* `GPIO 34` -> tín hiệu analog mô phỏng MQ3
* `GPIO 15` -> servo SG90
* `GPIO 13` -> buzzer
* `GPIO 25` -> LED đỏ
* `GPIO 26` -> LED xanh
* `GPIO 27` -> LED vàng
* `GPIO 14` -> nút TEST
* `GPIO 12` -> nút START
* `GPIO 21` -> OLED SDA
* `GPIO 22` -> OLED SCL

---

## 5. Linh kiện / thành phần đang dùng trong bản mô phỏng

Trong Wokwi, hệ thống hiện đang mô phỏng bằng:

* ESP32 DevKit V1
* OLED SSD1306 I2C
* Servo SG90
* Active buzzer
* 3 LED: đỏ, xanh, vàng
* 2 pushbutton: TEST, START
* 1 potentiometer để giả lập cảm biến MQ3

Lưu ý:

* **không dùng MQ3 thật trên Wokwi**
* dùng **potentiometer** để tạo giá trị analog vào GPIO34
* hướng này đã được chốt để demo web cho ổn định

---

## 6. Công nghệ và hướng code đã chốt

Code hiện tại đi theo hướng:

* **ESP32 Arduino framework**
* ngôn ngữ thực tế: **C++ dạng `.ino`**
* sau này nếu chuyển sang local VS Code thì có thể chuyển thành `src/main.cpp` trong PlatformIO

Các thư viện đang dùng:

* `Wire.h`
* `Adafruit_GFX.h`
* `Adafruit_SSD1306.h`
* `ESP32Servo.h`

---

## 7. Hành vi mong muốn của giao diện / thiết bị

### OLED

Hiển thị trạng thái hệ thống:

* đang khởi động / preheat
* chờ đo
* đang lấy mẫu
* pass / fail
* đã mở khóa / đang chạy

### LED

* vàng: preheat / khởi tạo
* xanh: pass
* đỏ: fail

### Buzzer

* kêu khi fail
* không kêu khi pass

### Servo

* góc khóa: `0 độ`
* góc mở: `90 độ`

---

## 8. Tham số hiện tại để demo

Các tham số đang dùng để demo:

* `PREHEAT_MS = 10000`
* `ALCOHOL_THRESHOLD = 2000`
* lấy mẫu nhiều lần rồi tính trung bình

Ghi chú:

* `PREHEAT_MS` đã được giảm xuống để demo nhanh
* `ALCOHOL_THRESHOLD` hiện là **ngưỡng demo**, có thể cần chỉnh lại để phù hợp với biên độ của potentiometer trong Wokwi

---

## 9. Những gì cần Codex hỗ trợ tiếp

Codex cần hỗ trợ theo các hướng sau:

1. rà soát lại toàn bộ code hiện tại
2. đảm bảo state machine chạy ổn định, không kẹt trạng thái
3. tối ưu lại text hiển thị trên OLED cho rõ ràng, dễ demo
4. thêm log Serial để debug giá trị ADC, trung bình mẫu, kết quả PASS/FAIL
5. kiểm tra debounce cho nút bấm
6. đảm bảo START chỉ hoạt động khi đã PASS
7. đảm bảo FAIL thì servo luôn giữ khóa
8. có thể bổ sung một flow reset / stop demo nếu cần
9. giữ code gọn, rõ ràng, dễ trình bày cho đồ án học phần

---

## 10. Ràng buộc quan trọng

* đây là **prototype học phần**, không cần đạt chuẩn thiết bị thương mại
* không cần xử lý relay ở giai đoạn hiện tại
* **servo là chấp hành chính**
* **potentiometer thay MQ3 trên Wokwi**
* ưu tiên code dễ đọc, dễ demo, dễ giải thích với giảng viên
* không cần làm web/app/cloud, chỉ tập trung vào hệ thống nhúng và mô phỏng

---

## 11. Mục tiêu cuối cùng trước mắt

Tôi cần một bản mô phỏng chạy ổn định để demo được 3 tình huống:

1. **Boot / Preheat**
2. **PASS rồi START**
3. **FAIL rồi không cho START**

Sau đó mới tính đến:

* hoàn thiện code sạch hơn
* chuẩn bị sơ đồ nối dây thật
* chỉnh báo cáo và slide thuyết trình

---

## 12. Yêu cầu với Codex

Hãy hỗ trợ tôi như một kỹ sư nhúng đang hoàn thiện prototype cho đồ án:

* đọc và hiểu đúng bối cảnh ở trên
* không tự ý đổi hướng kiến trúc
* giữ nguyên pin mapping nếu không thật sự cần đổi
* ưu tiên sửa trực tiếp trên code hiện tại
* nếu đề xuất thay đổi, hãy nói rõ lý do và tác động
* ưu tiên demo ổn định trên Wokwi / ESP32 Arduino trước

---
