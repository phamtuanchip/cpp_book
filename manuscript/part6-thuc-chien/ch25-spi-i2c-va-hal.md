---
chapter: 25
title: "SPI, I2C và thiết kế lớp trừu tượng phần cứng (HAL)"
part: 6
code: code/chapter-25
---

# Chương 25. SPI, I2C và thiết kế HAL

## Mục tiêu
- Thiết kế API SPI theo **giao dịch (transaction)**, dùng RAII để chip-select luôn được thả đúng lúc.
- Xây một HAL mỏng bằng `concept` + template: driver cảm biến không biết (và không cần biết) nó chạy trên bus thật hay bus giả lập.
- Biết khi nào chọn HAL kiểu template (chương 15/17) so với HAL kiểu lớp ảo (chương 14) cho I2C/SPI cụ thể.

## Câu chuyện: quên thả chip-select khi có lỗi giữa chừng
Driver SPI viết tay: `cs_low(); spi_transfer(...); cs_high();`. Một ngày, `spi_transfer` gặp lỗi và hàm gọi nó `return` sớm để báo lỗi — bỏ qua dòng `cs_high()`. Chip vẫn bị "chọn" mãi mãi, mọi giao tiếp SPI sau đó với thiết bị khác trên cùng bus đều sai, và lỗi chỉ xuất hiện **sau khi** đường lỗi đầu tiên xảy ra — rất khó tái hiện. Đây là bài toán RAII kinh điển (chương 12): quản lý tài nguyên bằng cách gắn nó với vòng đời đối tượng, không phải bằng kỷ luật "nhớ gọi hàm dọn dẹp".

## Kiến thức

### 1. Chip-select là tài nguyên, không phải hai lệnh rời rạc

{{code:chapter-25/spi_transaction.cpp}}

{{out:chapter-25/spi_transaction}}

`ChipSelectGuard` không có gì đặc biệt — đúng RAII cơ bản của chương 12 — nhưng áp dụng đúng chỗ nó giải quyết một lớp bug thật: **mọi đường thoát khỏi `spi_transaction`** (thành công, `return` sớm khi lỗi, sau này nếu thêm `throw`) đều chạy qua destructor của `cs`, CS luôn được thả. Ví dụ minh hoạ hai đường thoát (thành công và lỗi kích thước) đều tha CS đúng.

API nhận `std::span` (chương 10) thay vì con trỏ + độ dài rời: người gọi không thể truyền lệch độ dài `tx`/`rx` mà không bị runtime kiểm tra ngay trong hàm (ở đây kiểm tra thủ công; `span` giúp không quên độ dài đi kèm con trỏ như bug cổ điển).

### 2. HAL bằng `concept` + template (biết bus lúc biên dịch)

{{code:chapter-25/i2c_hal_concept.cpp}}

{{out:chapter-25/i2c_hal_concept}}

`concept I2cBus` định nghĩa đúng những gì `TempSensor` cần: `write_reg`/`read_reg` với đúng chữ ký. `TempSensor<Bus>` chấp nhận **bất kỳ kiểu nào** thoả `concept` đó — `FakeI2cBus` trong ví dụ để chạy trên host, hoặc một driver I2C phần cứng thật trên MCU, miễn đúng giao diện. Đây là HAL theo đúng nghĩa: `TempSensor` không `#include` gì liên quan tới phần cứng cụ thể, không có `#ifdef BOARD_X`.

So với dùng con trỏ hàm hoặc lớp cơ sở ảo cho `Bus`: cách này giải quyết **lúc biên dịch** (giống lựa chọn CRTP ở chương 15) — không có chi phí gọi gián tiếp, và nếu `FakeI2cBus` (hoặc driver thật) thiếu một hàm, lỗi hiện ra **ngay tại điểm gọi tạo `TempSensor`** với thông báo rõ ràng nhờ `concept`, không phải lỗi khó đọc từ sâu trong thân template.

### 3. Khi nào HAL cần lớp ảo thay vì template
Bài toán đổi khi:
- **Chọn bus lúc chạy**: ví dụ driver hỗ trợ cả cảm biến trên I2C lẫn SPI, và loại bus được đọc từ file cấu hình lúc khởi động, không biết trước lúc biên dịch.
- **Thư viện phân phối dạng biên dịch sẵn** (không phải header-only) cần một ABI ổn định không phụ thuộc kiểu template cụ thể của người dùng.

Trong các trường hợp đó, quay lại bảng quyết định ở chương 15 mục 3: dùng lớp cơ sở ảo `II2cBus` với `write_reg`/`read_reg` ảo, đánh đổi lấy một lần gọi gián tiếp cho mỗi giao dịch — thường không đáng kể so với thời gian truyền vật lý qua bus I2C/SPI (hàng micro giây tới mili giây), vốn đã chậm hơn hàng trăm lần một lời gọi hàm ảo.

## Góc nhúng
- I2C/SPI vật lý chậm hơn CPU rất nhiều bậc (kHz–MHz so với hàng chục–hàng trăm MHz của lõi) — chi phí gọi gián tiếp (vtable) gần như luôn **không đáng lo** ở tầng driver bus, dù có đáng lo ở tầng gọi liên tục trong vòng lặp nóng (chương 15).
- `ChipSelectGuard` nên dùng chân GPIO thật qua `GpioPin` (chương 23) trong code sản phẩm; ví dụ ở đây dùng biến `bool` để chạy được trên host.
- Khi nhiều thiết bị dùng chung một bus SPI, `unique_ptr` + custom deleter hoặc handle move-only (chương 18) có thể quản lý quyền "đang chiếm bus" giữa các driver.

## Lỗi thường gặp
- Quên RAII cho chip-select, dựa vào kỷ luật gọi `cs_high()` ở mọi đường thoát — chỉ cần quên một chỗ (thường là nhánh lỗi hiếm gặp) là để lại lỗi khó tái hiện.
- Viết `concept` quá lỏng (chỉ kiểm tra tên hàm tồn tại, không kiểm tra kiểu trả về) khiến lỗi kiểu sai vẫn lọt qua rồi báo lỗi khó đọc từ bên trong.
- Trộn lẫn nhiều thiết bị SPI khác tốc độ/chế độ (mode 0-3, tốc độ clock) mà không cấu hình lại thanh ghi SPI giữa các giao dịch tới từng chip.
- Dùng HAL kiểu template rồi vẫn cố chọn bus lúc chạy bằng `if`/`switch` bên trong — mất hết lợi ích giải quyết lúc biên dịch mà vẫn chịu chi phí nhánh rẽ.

## Bài tập
1. Thêm phương thức `transaction_count()` vào `ChipSelectGuard` (qua một biến tĩnh) để đếm tổng số giao dịch SPI đã thực hiện.
2. Mở rộng `concept I2cBus` để yêu cầu thêm `bus.is_busy() -> bool`; cập nhật `FakeI2cBus` và `TempSensor` cho phù hợp.
3. Viết một `FaultyI2cBus` thiếu hàm `read_reg` và thử khởi tạo `TempSensor<FaultyI2cBus>` — đọc thông báo lỗi do `concept` sinh ra, so sánh với việc bỏ `concept` (dùng `typename Bus` trần) và gây lỗi tương tự.
4. Viết bằng lời: vì sao chi phí một lời gọi hàm ảo thường "biến mất" so với thời gian một giao dịch I2C ở tốc độ chuẩn 100 kHz, kèm một phép tính ước lượng đơn giản (số chu kỳ CPU cho một lời gọi ảo so với thời gian truyền 1 byte ở 100 kHz).

## Tóm tắt
- Chip-select (và tài nguyên bus nói chung) nên là một đối tượng RAII, không phải hai lệnh rời rạc dễ quên.
- `concept` + template cho HAL giải quyết lúc biên dịch, không chi phí gọi gián tiếp, lỗi rõ ràng khi driver không khớp giao diện.
- Chọn lớp ảo cho HAL khi cần chọn bus/thiết bị lúc chạy — chi phí gọi gián tiếp thường không đáng kể so với tốc độ vật lý của I2C/SPI.
