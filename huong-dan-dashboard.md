# Hướng Dẫn Dashboard Realtime

## 1. Mục tiêu

Tài liệu này mô tả luồng end-to-end mới:

- `ESP32` gửi telemetry và sự kiện qua `Serial USB`
- `Node.js backend` đọc serial và phát realtime bằng `WebSocket`
- `Web dashboard` hiển thị trạng thái, lịch sử ADC và gửi `browser notification`

Mô hình này bám sát prototype hiện tại:
- không cần Wi-Fi trên ESP32
- không cần cloud
- vẫn dùng state machine và phần cứng đang có

## 2. Kiến trúc

Luồng dữ liệu:

1. ESP32 chạy firmware hiện tại
2. Firmware phát ra 2 loại dòng serial:
   - `AI_JSON` telemetry
   - `AI_JSON` event
3. `dashboard/server.js` đọc các dòng này từ serial
4. Server lưu snapshot hiện tại và đẩy lên trình duyệt bằng `socket.io`
5. `dashboard/public/app.js` cập nhật UI realtime
6. Trình duyệt phát thông báo khi có `PASS`, `FAIL`, lỗi kết nối hoặc cảnh báo

## 3. Những gì đã được sửa trong firmware

Firmware hiện tại ngoài phần OLED/LED/servo/buzzer còn có thêm:

- `ENABLE_DASHBOARD_PROTOCOL = true`
- telemetry định kỳ
- sự kiện trạng thái và kết quả mẫu
- debug `Live ADC` cho dễ chỉnh biến trở

Firmware sẽ phát các dòng dạng:

```text
AI_JSON {"type":"telemetry", ...}
AI_JSON {"type":"event", ...}
```

Node backend chỉ parse các dòng có tiền tố `AI_JSON`, nên các log debug bình thường vẫn có thể giữ nguyên.

## 4. Cấu trúc file mới

- `package.json`: dependency và script cho dashboard
- `dashboard/server.js`: backend đọc serial + phát realtime
- `dashboard/public/index.html`: giao diện dashboard
- `dashboard/public/styles.css`: giao diện hiển thị
- `dashboard/public/app.js`: logic realtime, chart, notifications
- `src/main.cpp`: entry cho PlatformIO, include lại `sketch.ino`

## 5. Cài dependency

Chạy một lần ở thư mục project:

```powershell
npm install
```

## 6. Build firmware

Mỗi khi sửa `sketch.ino`, cần build lại:

```powershell
pio run
```

Firmware build ra tại:
- `.pio/build/esp32dev/firmware.bin`
- `.pio/build/esp32dev/firmware.elf`

## 7. Chạy dashboard với ESP32 thật

### Bước 1: cắm ESP32

- kết nối ESP32 với máy tính qua USB
- đảm bảo firmware mới đã được nạp

### Bước 2: tìm cổng serial

Nếu chưa biết cổng COM nào:

```powershell
npm run ports
```

### Bước 3: chạy server

Nếu muốn để server tự dò cổng:

```powershell
npm run start
```

Nếu muốn chỉ định cổng cụ thể:

```powershell
node dashboard/server.js --serial COM5
```

### Bước 4: mở dashboard

Mở trình duyệt tại:

```text
http://localhost:3030
```

### Bước 5: bật thông báo trình duyệt

Trong dashboard:
- bấm `Bật thông báo`
- cho phép browser notification

## 8. Chạy dashboard ở mock mode

Nếu chưa cắm ESP32 mà vẫn muốn test UI:

```powershell
npm run mock
```

Dashboard sẽ tự tạo dữ liệu mẫu PASS / RUNNING / FAIL theo chu kỳ.

## 9. Cách demo end-to-end

### Demo với ESP32 / mô hình thật

1. Nạp firmware mới
2. Cắm ESP32 vào máy
3. Chạy `npm run start`
4. Mở `http://localhost:3030`
5. Bật notification
6. Chỉnh giá trị cảm biến / mô phỏng MQ3
7. Nhấn `TEST`
8. Quan sát:
   - dashboard cập nhật `Live ADC`
   - event mới xuất hiện
   - browser notification bật lên nếu có `PASS` hoặc `FAIL`
9. Nhấn `START`
10. Quan sát trạng thái `RUNNING` và góc servo trên dashboard

### Demo với Wokwi

Luồng dashboard hiện tối ưu cho:
- ESP32 thật qua USB serial

Wokwi vẫn dùng tốt cho mô phỏng embedded, nhưng dashboard không lấy dữ liệu trực tiếp từ cửa sổ Wokwi. Nếu cần demo web mà chưa cắm board thật, dùng:

```powershell
npm run mock
```

## 10. Những gì dashboard hiển thị

- trạng thái kết nối serial
- cổng COM đang dùng
- `Live ADC`
- ngưỡng hiện tại
- mẫu ADC cuối cùng
- trạng thái state machine
- kết quả `PASS` / `FAIL`
- servo angle
- khóa xe / mở khóa
- buzzer on/off
- countdown preheat
- lịch sử sự kiện gần nhất
- raw serial line gần nhất

## 11. Khi nào cần build lại

### Cần build lại nếu:

- bạn sửa `sketch.ino`
- bạn đổi pin
- bạn đổi ngưỡng
- bạn đổi logic nút
- bạn đổi giao thức serial / event / telemetry

### Không cần build lại nếu:

- bạn chỉ sửa `dashboard/server.js`
- bạn chỉ sửa UI trong `dashboard/public/*`
- bạn chỉ đổi tài liệu `.md`

## 12. Task nhanh trong VS Code

Project đã có task:

- `Build Demo Firmware`
- `Clean Demo Build`
- `Start Dashboard`
- `Start Dashboard (Mock)`

Bạn có thể chạy từ:
- `Ctrl + Shift + P`
- `Tasks: Run Task`

## 13. Xử lý lỗi thường gặp

### Dashboard không nhận dữ liệu

Kiểm tra:
- firmware mới đã được nạp chưa
- ESP32 đã cắm USB chưa
- chọn đúng COM port chưa
- serial baud đang là `115200`

### Dashboard báo stale / disconnected

Nguyên nhân thường là:
- dây USB chập chờn
- board reset
- server đang nghe sai cổng COM

### Browser không hiện notification

Kiểm tra:
- đã bấm `Bật thông báo` chưa
- browser đã cấp quyền chưa
- tab dashboard có đang bị chặn notification không

## 14. Kết luận

Bản triển khai hiện tại đã đủ cho demo end-to-end theo hướng:

- phần nhúng vẫn là trung tâm
- web chỉ làm lớp giám sát realtime
- dữ liệu cập nhật tức thời
- có thông báo ngay khi có kết quả hoặc sự cố
