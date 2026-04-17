# Hướng Dẫn Nối Mạch Thực Tế

## 1. Mục tiêu

Tài liệu này hướng dẫn cách nối phần cứng thật cho project `Alcohol Interlock` trên `ESP32`, đồng thời giải thích:
- vì sao đã đổi từ nút nhấn thường sang module nút `3 chân`
- code hiện tại đang xử lý module `3 chân` như thế nào
- khi nào cần sửa code
- khi nào cần build lại `PlatformIO`

## 2. Tóm tắt thay đổi đã thực hiện

Ban đầu project mô phỏng bằng nút nhấn thường trong Wokwi.

Hiện tại project đã được sửa để phù hợp với module nút `3 chân` có:
- `VCC`
- `GND`
- `OUT`

Firmware hiện tại đang giả định:
- nhấn nút thì chân `OUT` lên mức `HIGH`
- thả nút thì chân `OUT` ở mức `LOW`

Tức là code đang chạy theo kiểu:
- `active HIGH`

Ngoài ra, chân `START` đã được đổi từ `GPIO12` sang `GPIO16`.

Lý do:
- `GPIO12` trên ESP32 là chân nhạy trong quá trình boot
- dùng `GPIO16` an toàn hơn cho mô hình thật

## 3. Code hiện tại đang hiểu nút 3 chân ra sao

Trong `sketch.ino`, cấu hình hiện tại là:
- `USE_3PIN_BUTTON_MODULES = true`
- `BUTTON_ACTIVE_HIGH = true`

Ý nghĩa:
- firmware đang bật chế độ đọc module `3 chân`
- nếu nhấn nút mà `OUT` lên `HIGH` thì hệ thống sẽ hiểu là đã nhấn

Nếu module của bạn là loại ngược lại:
- nhấn nút thì `OUT = LOW`

thì cần sửa:

```cpp
static const bool BUTTON_ACTIVE_HIGH = false;
```

Sau đó build lại:

```powershell
pio run
```

## 4. Diagram và mạch thật khác nhau thế nào

`diagram.json` chỉ dùng cho mô phỏng Wokwi.

Trong Wokwi hiện tại:
- vẫn dùng part `pushbutton`
- nhưng cách nối dây đã được sửa để mô phỏng hành vi tương đương module `3 chân`

Điều đó có nghĩa:
- mô phỏng đang đúng logic với phần cứng thật
- nhưng ngoài đời bạn **không cần** phải làm y hệt `diagram.json`
- ngoài đời bạn chỉ cần nối đúng `VCC`, `GND`, `OUT` của module vào ESP32

## 5. Sơ đồ nối chân cho phần cứng thật

### 5.1. Module nút TEST

- `VCC -> 3V3`
- `GND -> GND`
- `OUT -> GPIO14`

### 5.2. Module nút START

- `VCC -> 3V3`
- `GND -> GND`
- `OUT -> GPIO16`

### 5.3. OLED SSD1306

- `VCC -> 3V3`
- `GND -> GND`
- `SDA -> GPIO21`
- `SCL -> GPIO22`

### 5.4. Servo

- `SIG -> GPIO15`
- `V+ -> 5V`
- `GND -> GND`

Lưu ý:
- nên cấp nguồn `5V` riêng hoặc nguồn đủ dòng cho servo
- bắt buộc nối `GND` servo chung với `GND` của ESP32

### 5.5. Buzzer

- `SIG -> GPIO13`
- `GND -> GND`

Nếu buzzer của bạn có 3 chân:
- nối theo chân `S/OUT` của module vào `GPIO13`
- `VCC` và `GND` nối theo module đó

### 5.6. LED đỏ

- `GPIO25 -> điện trở 220-330 ohm -> chân dương LED`
- chân âm LED -> `GND`

### 5.7. LED xanh

- `GPIO26 -> điện trở 220-330 ohm -> chân dương LED`
- chân âm LED -> `GND`

### 5.8. LED vàng

- `GPIO27 -> điện trở 220-330 ohm -> chân dương LED`
- chân âm LED -> `GND`

### 5.9. MQ3 hoặc tín hiệu analog mô phỏng

- `AO -> GPIO34`
- `GND -> GND`
- `VCC -> theo module`

Lưu ý rất quan trọng:
- `GPIO34` là chân input-only, dùng để đọc analog là đúng
- điện áp đưa vào `GPIO34` không được vượt `3.3V`
- nếu module MQ3 của bạn xuất analog `0-5V`, cần mạch chia áp trước khi đưa vào ESP32

## 6. Bảng nối mạch nhanh

| Thiết bị | Chân thiết bị | ESP32 |
|---|---|---|
| TEST 3 chân | VCC | 3V3 |
| TEST 3 chân | GND | GND |
| TEST 3 chân | OUT | GPIO14 |
| START 3 chân | VCC | 3V3 |
| START 3 chân | GND | GND |
| START 3 chân | OUT | GPIO16 |
| OLED | VCC | 3V3 |
| OLED | GND | GND |
| OLED | SDA | GPIO21 |
| OLED | SCL | GPIO22 |
| Servo | SIG | GPIO15 |
| Servo | V+ | 5V |
| Servo | GND | GND |
| Buzzer | SIG/OUT | GPIO13 |
| Buzzer | GND | GND |
| LED đỏ | Anode qua điện trở | GPIO25 |
| LED xanh | Anode qua điện trở | GPIO26 |
| LED vàng | Anode qua điện trở | GPIO27 |
| MQ3 | AO | GPIO34 |
| MQ3 | GND | GND |

## 7. Có cần điện trở kéo lên/kéo xuống ngoài đời không

Thông thường:
- nếu bạn đang dùng đúng `module nút 3 chân` hoàn chỉnh
- module đã có mạch xử lý sẵn

thì bạn **không cần** tự thêm điện trở kéo lên/kéo xuống như trong Wokwi.

Phần điện trở trong `diagram.json` chỉ để làm cho nút nhấn thường trong Wokwi hoạt động giống module `3 chân`.

## 8. Có cần sửa code nữa không

### Không cần sửa nếu:

- module `TEST` và `START` của bạn là loại:
  - cấp `VCC`, `GND`
  - khi nhấn thì `OUT` lên `HIGH`

Khi đó:
- cứ dùng firmware hiện tại
- nạp vào ESP32 là chạy

### Cần sửa nếu:

module của bạn là loại:
- khi nhấn thì `OUT = LOW`

Khi đó:
- sửa `BUTTON_ACTIVE_HIGH = false`
- build lại `pio run`
- nạp lại firmware

## 9. Có cần build lại không

### Không cần build lại nếu:

- bạn chưa sửa thêm `sketch.ino`
- bạn đang dùng đúng cấu hình hiện tại:
  - `TEST -> GPIO14`
  - `START -> GPIO16`
  - module nút `active HIGH`

### Cần build lại nếu:

- bạn đổi chân nối
- bạn đổi cực tính nút
- bạn sửa logic code

Lệnh build:

```powershell
pio run
```

## 10. Cách nạp và chạy demo

Sau khi nối mạch xong:

1. Kết nối ESP32 với máy tính
2. Nạp firmware bằng cách bạn đang dùng hoặc dùng PlatformIO
3. Mở serial monitor nếu cần xem log
4. Cấp nguồn cho mạch
5. Chờ khoảng `10 giây` preheat

Demo nhánh `PASS`:
- giữ giá trị MQ3 ở mức thấp
- nhấn `TEST`
- hệ thống báo an toàn
- nhấn `START`
- servo mở khóa, hệ thống vào trạng thái chạy

Demo nhánh `FAIL`:
- tăng giá trị MQ3 vượt ngưỡng
- nhấn `TEST`
- hệ thống báo vi phạm
- LED đỏ sáng
- buzzer kêu
- servo không mở khóa

## 11. Checklist test thực tế

Bạn nên test theo thứ tự này:

1. OLED có lên hình không
2. LED vàng có sáng trong giai đoạn preheat không
3. Nhấn `TEST` có được nhận không
4. Nhấn `START` có được nhận không
5. Nhánh `PASS` có mở servo không
6. Nhánh `FAIL` có khóa servo và bật buzzer không
7. Khi đang chạy, nhấn `START` lần nữa có quay lại trạng thái khóa không

## 12. Nếu nút không hoạt động

Kiểm tra theo thứ tự:

1. Đã nối đúng `OUT` của TEST vào `GPIO14` chưa
2. Đã nối đúng `OUT` của START vào `GPIO16` chưa
3. `VCC` của module nút có đang vào `3V3` không
4. `GND` đã nối chung chưa
5. Module của bạn là `active HIGH` hay `active LOW`

Nếu nghi ngờ là `active LOW`:
- đổi `BUTTON_ACTIVE_HIGH` thành `false`
- build lại firmware
- nạp lại ESP32

## 13. Kết luận

Bản code hiện tại đã được chuẩn hóa cho mô hình thật dùng module nút `3 chân`.

Thiết lập mặc định hiện tại là:
- `TEST -> GPIO14`
- `START -> GPIO16`
- module nút `active HIGH`

Nếu phần cứng của bạn đúng kiểu đó thì:
- không cần sửa thêm code
- không cần sửa thêm diagram để chạy mạch thật
- chỉ cần nối đúng dây và nạp firmware hiện tại
