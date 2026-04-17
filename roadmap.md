# Roadmap Dự Án Alcohol Interlock Demo

## 1. Mục đích tài liệu

Tài liệu này là `source of truth` hiện tại cho dự án, dùng để thống nhất:
- mục tiêu demo
- hiện trạng code
- cấu hình chạy local trên `VS Code`
- phạm vi chức năng đã có
- hướng phát triển tiếp theo

Lưu ý:
- File `codex.md` hiện đang rỗng trên đĩa, nên nội dung trong roadmap này được tổng hợp từ trạng thái thực tế của repository và kết quả setup đã kiểm chứng local.
- Nếu sau này bạn cập nhật requirement chính thức vào `codex.md`, cần đồng bộ lại `roadmap.md`.

## 2. Tổng quan dự án

Đây là dự án mô phỏng `hệ thống khóa liên động chống khởi động xe khi phát hiện nồng độ cồn`, chạy trên:
- `ESP32`
- framework `Arduino`
- mô phỏng bằng `Wokwi`

Ý tưởng hoạt động:
- Hệ thống đọc giá trị analog mô phỏng từ cảm biến cồn `MQ3`
- Nếu giá trị dưới ngưỡng an toàn, cho phép người dùng khởi động
- Nếu giá trị vượt ngưỡng, hệ thống giữ trạng thái khóa và phát cảnh báo

## 3. Phần cứng mô phỏng hiện tại

Mạch mô phỏng trong `diagram.json` hiện gồm:
- `ESP32 DevKit V1`
- `OLED SSD1306`
- `Servo`
- `Buzzer`
- `LED đỏ`
- `LED xanh lá`
- `LED vàng`
- `Module TEST 3 chân` (`VCC`, `GND`, `OUT`)
- `Module START 3 chân` (`VCC`, `GND`, `OUT`)
- `Biến trở` đóng vai trò mô phỏng đầu ra analog của `MQ3`

## 4. Chức năng hiện đã có

Firmware hiện tại đã có đầy đủ luồng demo cơ bản:
- Giai đoạn `preheat` cho cảm biến
- Trạng thái chờ khi xe đang bị khóa
- Nhấn `TEST` để bắt đầu đo
- Lấy mẫu analog và tính giá trị trung bình
- Nếu dưới ngưỡng thì `PASS`
- Nếu vượt ngưỡng thì `FAIL`
- Khi `PASS`, nhấn `START` để mở khóa và chuyển sang trạng thái đang chạy
- Khi đang chạy, nhấn `START` thêm lần nữa để dừng demo và quay lại trạng thái khóa

Các tín hiệu phản hồi đang có:
- `OLED` hiển thị trạng thái hệ thống
- `LED vàng` cho preheat/sampling
- `LED xanh` cho trạng thái đạt
- `LED đỏ` cho trạng thái vi phạm
- `Buzzer` cảnh báo khi fail
- `Servo` mô phỏng cơ cấu khóa/mở khóa

## 5. Cấu hình demo hiện tại

Trong `sketch.ino`, cấu hình hiện đang tối ưu cho demo:
- `DEMO_MODE = true`
- `PREHEAT_MS = 10000`
- `ALCOHOL_THRESHOLD = 2000`

Ý nghĩa:
- Thời gian preheat đang để `10 giây` để trình diễn nhanh
- Nếu cần sát hơn với yêu cầu bài toán hoặc phần cứng thật, có thể tắt `DEMO_MODE` để quay lại `60 giây`

## 6. Vấn đề ban đầu và cách xử lý

Vấn đề bạn gặp lúc đầu:
- Wokwi trên web báo build chậm hoặc bị kẹt queue
- Giao diện hiển thị như thể project quá lớn hoặc server quá tải

Kết luận sau khi kiểm tra local:
- Project **không quá lớn**
- Lỗi đó đến từ môi trường build online của Wokwi, không phải do firmware vượt giới hạn ESP32

Kết quả build local đã xác nhận:
- `RAM`: khoảng `7.1%`
- `Flash`: khoảng `24.3%`

## 7. Setup local đã hoàn tất

Để bỏ phụ thuộc vào hàng đợi compile của Wokwi web, project đã được cấu hình chạy local trên `VS Code`.

Các file đã được thêm:
- `platformio.ini`
- `wokwi.toml`
- `.vscode/extensions.json`
- `.vscode/tasks.json`

Ý nghĩa từng file:
- `platformio.ini`: cấu hình build local cho `ESP32 + Arduino`
- `wokwi.toml`: chỉ đường dẫn firmware để Wokwi VS Code nạp khi mô phỏng
- `.vscode/extensions.json`: gợi ý extension cần cài
- `.vscode/tasks.json`: tạo task build/clean để thao tác nhanh trong VS Code

## 8. Tương thích môi trường

Máy hiện tại đang dùng:
- `PlatformIO Core 6.1.18`
- `Python 3.9.0`

Do `Python 3.9.0` gây lỗi với một số dependency mới của `tool-esptoolpy`, project đang được ghim về:
- `espressif32@6.3.2`

Mục đích:
- đảm bảo build ổn định trên chính môi trường hiện tại
- tránh việc phải sửa hệ thống Python ngay lúc chuẩn bị demo

## 9. Lỗi code đã được vá

Trong quá trình chuyển sang build local, đã phát hiện và xử lý lỗi compile thực tế:
- `ButtonTracker` khởi tạo theo kiểu không tương thích với toolchain local

Đã sửa theo hướng:
- thêm constructor rõ ràng cho `ButtonTracker`
- khởi tạo `btnTest` và `btnStart` theo kiểu ổn định hơn

Ngoài ra:
- đã bật `DEMO_MODE` để rút ngắn thời gian chờ demo

## 10. Sơ đồ trạng thái nghiệp vụ

Luồng trạng thái hiện tại:

1. `STATE_PREHEAT`
2. `STATE_STANDBY_LOCKED`
3. `STATE_SAMPLING`
4. `STATE_PASS_READY` hoặc `STATE_FAIL_LOCKED`
5. Nếu pass và nhấn `START` thì sang `STATE_RUNNING`
6. Khi đang chạy, nhấn `START` lần nữa để quay lại `STATE_STANDBY_LOCKED`

Ý nghĩa thực tế:
- xe chỉ được phép khởi động sau khi đo đạt yêu cầu
- nếu đo fail thì chỉ cho đo lại, không cho mở khóa

## 11. Mapping chân hiện tại

Các chân quan trọng:
- `MQ3 analog` -> `GPIO34`
- `Servo` -> `GPIO15`
- `Buzzer` -> `GPIO13`
- `LED đỏ` -> `GPIO25`
- `LED xanh` -> `GPIO26`
- `LED vàng` -> `GPIO27`
- `OUT TEST` -> `GPIO14`
- `OUT START` -> `GPIO16`
- `OLED SDA` -> `GPIO21`
- `OLED SCL` -> `GPIO22`

Lưu ý:
- Trong mô phỏng, `MQ3` đang được thay bằng `potentiometer`
- Hai nút `TEST/START` hiện đã được quy đổi sang logic module `3 chân`
- Mặc định firmware đang giả định `OUT = HIGH` khi nhấn

## 12. Các file quan trọng nhất

- `sketch.ino`: firmware chính
- `diagram.json`: sơ đồ mạch Wokwi
- `libraries.txt`: danh sách thư viện cho Wokwi
- `platformio.ini`: cấu hình build local
- `wokwi.toml`: cấu hình mô phỏng local
- `.vscode/tasks.json`: task build/clean trong VS Code
- `huong-dan-chay.md`: hướng dẫn chạy demo từng bước

## 13. Kịch bản demo đề xuất

Kịch bản trình bày ngắn gọn:

1. Mở simulator
2. Chờ `10 giây` preheat
3. Để biến trở ở mức thấp
4. Nhấn `TEST`
5. Hệ thống hiển thị `PASS`
6. Nhấn `START` để mở khóa
7. Nhấn `START` lần nữa để quay lại trạng thái khóa
8. Tăng biến trở lên cao hơn ngưỡng
9. Nhấn `TEST`
10. Hệ thống hiển thị `FAIL`, còi kêu và xe vẫn bị khóa

## 14. Giới hạn hiện tại

Một số giới hạn cần ghi nhận:
- `codex.md` hiện chưa có nội dung requirement chính thức
- Dữ liệu `MQ3` mới chỉ là mô phỏng analog, chưa có calibration thực tế
- Chức năng `relay` đã có khung nhưng đang tắt
- Luồng Wokwi trong VS Code vẫn cần internet và license Wokwi
- Hệ thống hiện đang tối ưu cho demo, chưa phải bản hardware cuối

## 15. Hướng phát triển tiếp theo

Ngắn hạn:
- điền đầy đủ requirement vào `codex.md`
- chốt lại thời gian preheat cho mục tiêu báo cáo hay demo
- rà soát nội dung hiển thị OLED để nhất quán với bài thuyết trình

Trung hạn:
- thêm relay để mô phỏng khóa/ngắt động cơ sát thực tế hơn
- thay mô phỏng biến trở bằng logic gần hơn với cảm biến MQ3 thật
- bổ sung log serial hoặc hình ảnh minh họa cho báo cáo

Dài hạn:
- tách code thành module rõ hơn nếu project phát triển thêm
- thêm test scenario tự động nếu cần kiểm thử hành vi demo

## 16. Điều kiện hoàn thành giai đoạn hiện tại

Giai đoạn setup hiện tại được xem là hoàn thành khi:
- project build local thành công trong `VS Code`
- Wokwi trong `VS Code` nạp được firmware local
- demo được cả 2 nhánh `PASS` và `FAIL`
- tài liệu này tiếp tục được giữ đồng bộ với codebase thực tế
