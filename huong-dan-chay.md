# Hướng Dẫn Chạy Demo Trên VS Code

## 1. Mục tiêu

Tài liệu này hướng dẫn cách chạy project `Alcohol Interlock` trực tiếp trong `VS Code`, thay cho việc build trên Wokwi web.

Luồng này giúp bạn:
- tránh lỗi queue/build server của Wokwi web
- build firmware ngay trên máy
- dùng lại `diagram.json` để mô phỏng trong `Wokwi for VS Code`

## 2. Điều kiện cần

Bạn cần có:
- `VS Code`
- extension `PlatformIO IDE`
- extension `Wokwi for VS Code`
- internet
- license Wokwi cho bản VS Code

## 3. Mở project

Mở đúng folder project:

`d:\PTIT\Embedded System alcohol_interlock`

Sau khi mở project, bạn nên thấy các file chính:
- `sketch.ino`
- `diagram.json`
- `platformio.ini`
- `wokwi.toml`
- `roadmap.md`

## 4. Cài extension cần thiết

Trong VS Code:

1. Mở tab `Extensions`
2. Cài `PlatformIO IDE`
3. Cài `Wokwi for VS Code`

Project đã có file `.vscode/extensions.json` để gợi ý 2 extension này.

## 5. Build firmware local

Có 2 cách build:

### Cách 1: dùng task trong VS Code

1. Nhấn `Ctrl + Shift + B`
2. Chọn `Build Demo Firmware`

### Cách 2: dùng terminal

Mở terminal trong VS Code và chạy:

```powershell
pio run
```

Nếu build thành công, bạn sẽ có:
- `.pio/build/esp32dev/firmware.bin`
- `.pio/build/esp32dev/firmware.elf`

## 6. Chạy mô phỏng bằng Wokwi trong VS Code

Sau khi build xong:

1. Mở file `diagram.json`
2. Nhấn `F1`
3. Gõ `Wokwi: Start Simulator`
4. Chọn lệnh đó để chạy mô phỏng

Nếu là lần đầu dùng:

1. Nhấn `F1`
2. Gõ `Wokwi: Request a new License`
3. Làm theo hướng dẫn kích hoạt license
4. Sau đó chạy lại `Wokwi: Start Simulator`

## 7. Cách demo nhanh

Code hiện tại đang bật `DEMO_MODE = true`, nên:
- thời gian preheat khoảng `10 giây`

Demo nhánh `PASS`:

1. Chạy simulator
2. Chờ hết preheat
3. Vặn biến trở xuống mức thấp
4. Nhấn nút `TEST`
5. OLED sẽ báo đạt
6. Nhấn nút `START` để mở khóa

Demo nhánh `FAIL`:

1. Khi đang ở trạng thái khóa/chờ đo
2. Vặn biến trở lên mức cao
3. Nhấn `TEST`
4. OLED báo vi phạm
5. `LED đỏ` sáng, `buzzer` kêu, xe vẫn bị khóa

## 8. Nếu muốn build lại từ đầu

Bạn có thể clean trước rồi build lại:

### Dùng task

1. Mở `Command Palette`
2. Chạy task `Clean Demo Build`
3. Build lại bằng `Build Demo Firmware`

### Dùng terminal

```powershell
pio run -t clean
pio run
```

## 9. Một số lỗi thường gặp

### Lỗi 1: Wokwi web báo build chậm hoặc server bận

Nguyên nhân:
- server build online của Wokwi đang bận

Cách xử lý:
- không dùng Wokwi web để compile nữa
- build local bằng `PlatformIO`
- chạy mô phỏng qua `Wokwi for VS Code`

### Lỗi 2: Chưa chạy được simulator trong VS Code

Kiểm tra lại:
- đã cài `Wokwi for VS Code`
- đã build ra `firmware.bin` và `firmware.elf`
- file `wokwi.toml` còn đúng đường dẫn
- license Wokwi đã được kích hoạt

### Lỗi 3: Build lỗi do môi trường

Project hiện đã được ghim về `espressif32@6.3.2` để tương thích với máy hiện tại.

Vì vậy:
- không nên đổi `platformio.ini` nếu chưa thật sự cần

## 10. Muốn chạy trên phần cứng thật thì sao

Luồng hiện tại đang tối ưu cho mô phỏng demo.

Nếu chuyển sang phần cứng thật, cần làm thêm:
- thay biến trở mô phỏng bằng cảm biến `MQ3` thật
- hiệu chuẩn lại ngưỡng `ALCOHOL_THRESHOLD`
- cân nhắc dùng `relay` để khóa/ngắt động cơ
- kiểm tra nguồn cấp cho `servo`, `OLED`, `MQ3`
- nối `OUT` của module `TEST` vào `GPIO14`
- nối `OUT` của module `START` vào `GPIO12`
- nối `VCC` của 2 module nút vào `3V3`
- nối `GND` của 2 module nút vào `GND`

Lưu ý về logic nút:
- firmware hiện mặc định hiểu `nhấn nút = OUT lên HIGH`
- nếu module của bạn xuất `LOW` khi nhấn, đổi `BUTTON_ACTIVE_HIGH` trong `sketch.ino` thành `false`

## 11. Tóm tắt thao tác ngắn nhất

Nếu chỉ cần chạy thật nhanh:

1. Mở project trong `VS Code`
2. Cài `PlatformIO IDE`
3. Cài `Wokwi for VS Code`
4. Chạy `pio run`
5. Mở `diagram.json`
6. Chạy `Wokwi: Start Simulator`

## 12. File nên đọc cùng

- `roadmap.md`: mô tả tổng thể dự án và hiện trạng
- `sketch.ino`: code chính
- `diagram.json`: sơ đồ mô phỏng
