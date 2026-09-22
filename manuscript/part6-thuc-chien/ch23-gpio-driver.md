---
chapter: 23
title: "GPIO driver bằng C++: kết hợp MMIO và template"
part: 6
code: code/chapter-23
---

# Chương 23. GPIO driver bằng C++

## Mục tiêu
- Ghép MMIO (chương 21) với template tham số không phải kiểu (chương 15) thành một driver GPIO không chi phí runtime.
- Biến mỗi chân GPIO thành **một kiểu riêng**, giúp trình biên dịch bắt lỗi nhầm chân ngay lúc biên dịch.
- Biết đánh đổi giữa cách này và một class `GpioPin` thông thường nhận `(port, pin)` làm tham số hàm.

## Câu chuyện: driver GPIO đầu tiên, và cái giá của "linh hoạt"
Cách viết C quen thuộc: `void gpio_write(GPIO_TypeDef* port, uint8_t pin, bool high)`. Nó hoạt động, nhưng `pin` là số chạy lúc runtime — gọi `gpio_write(GPIOA, 99, true)` (chân không tồn tại) là lỗi logic chỉ phát hiện được lúc chạy, nếu có kiểm tra. Với template tham số không phải kiểu, `Pin` trở thành một phần của **kiểu**, và các phép tính offset/mask được trình biên dịch làm sẵn — không có "chân 99" nào lọt qua nếu bạn giới hạn kiểu đúng.

## Kiến thức

### 1. Mỗi chân là một kiểu

{{code:chapter-23/gpio_pin.cpp}}

{{out:chapter-23/gpio_pin}}

`GpioPin<&g_fake_porta, 5>` và `GpioPin<&g_fake_porta, 6>` là **hai kiểu khác nhau** dù cùng port — giống hệt cách CRTP ở chương 15 làm `UartDriver`/`SpiDriver` thành hai kiểu tách biệt. Hệ quả:
- Không thể gọi nhầm `LedPin::write` rồi tưởng đang điều khiển `ButtonPin` — chúng là API riêng biệt, không có tham số `pin` nào để gõ nhầm số.
- `set_mode`, `write`, `toggle`, `read` đều là hàm **static**: không cần tạo đối tượng, không tốn RAM cho "thể hiện" — toàn bộ trạng thái nằm ở thanh ghi phần cứng (`Port`), không phải trong đối tượng C++.
- Với `-O2`, `LedPin::write(true)` thường biên dịch thành đúng một lệnh ghi thanh ghi — không có lời gọi hàm thật, không có tham số truyền qua stack.

### 2. Đặt tên có nghĩa bằng `using`
`using LedPin = GpioPin<&g_fake_porta, 5>;` cho code gọi (`LedPin::write(true)`) đọc như tài liệu phần cứng, thay vì rải số 5 khắp nơi. Đổi board (chân LED khác) chỉ cần sửa **một dòng** `using`, không sửa logic gọi.

### 3. Đánh đổi: kiểu tĩnh so với tham số runtime
| | `GpioPin<Port, Pin>` (mỗi chân một kiểu) | `gpio_write(port, pin, ...)` (tham số runtime) |
|---|---|---|
| Nhầm chân | Bắt được lúc biên dịch (kiểu khác nhau) | Chỉ bắt được lúc chạy (nếu có kiểm tra) |
| Chọn chân lúc chạy (từ cấu hình, EEPROM) | Không làm được trực tiếp | Làm được |
| Mảng nhiều chân cùng loại | Cần kỹ thuật gộp kiểu (template pack, hoặc bọc bằng lớp ảo — chương 15 mục 3) | Tự nhiên (`for` qua mảng số pin) |
| Code sinh ra | Chuyên biệt cho từng chân, có thể phình nếu quá nhiều chân khác nhau dùng logic phức tạp | Dùng chung một hàm |

Với chân **cố định theo thiết kế phần cứng** (LED trạng thái, chip-select cụ thể), cách kiểu tĩnh gọn và an toàn hơn. Với chân **chọn lúc chạy** (ví dụ driver LED ma trận nhận số chân từ cấu hình người dùng), cần tham số runtime — quay lại nguyên tắc chọn ở chương 15 mục 3: biết lúc nào (biên dịch hay chạy) quyết định cách viết.

## Góc nhúng
- Kiểm tra bằng Compiler Explorer: biên dịch `LedPin::write(true)` với `-O2 --target arm` và xem có đúng là một lệnh `str`/`strb` duy nhất không.
- `set_mode`/`write`/`toggle` đọc-sửa-ghi thanh ghi `moder`/`odr` — nếu ISR cũng ghi cùng thanh ghi này, cần bảo vệ bằng critical section (chương 24) hoặc dùng thanh ghi set/reset riêng của MCU (`BSRR`) nếu có, để thao tác là một lệnh ghi nguyên tử phần cứng thay vì đọc-sửa-ghi.
- Với board có hàng chục chân, cân nhắc sinh code `using` cho toàn bộ pinout từ một bảng cấu hình (script hoặc `constexpr`), thay vì gõ tay từng dòng dễ sai.

## Lỗi thường gặp
- Quên `set_mode` trước khi `write`/`read` — chân vẫn ở chế độ mặc định (thường là input) và thao tác không có tác dụng như mong đợi.
- Đặt `Pin` vượt quá số bit thực có trên thanh ghi (ví dụ `Pin = 20` trên thanh ghi 16 chân) — không có `static_assert` chặn, hành vi undefined do dịch bit vượt quá kích thước kiểu.
- Nhầm lẫn `GpioPin<&g_fake_porta, 5>` và `GpioPin<&g_fake_portb, 5>` là "cùng chân 5" — chúng khác port, khác kiểu, khác thanh ghi hoàn toàn.
- Cố gán `LedPin` sang một biến kiểu `ButtonPin` — không biên dịch được (đúng như mong muốn, vì đây là hai kiểu khác nhau), nhưng người mới có thể hiểu nhầm là lỗi.

## Bài tập
1. Thêm `static_assert(Pin < 16, "So chan vuot qua thanh ghi 16-bit")` vào `GpioPin` và thử khởi tạo với `Pin = 20` để xem thông báo lỗi.
2. Viết `GpioPin::read_output()` trả về trạng thái hiện tại của `odr` cho chân đó (không đọc từ `idr`), hữu ích khi cần biết "mình vừa đặt LED thành gì" mà không phụ thuộc phần cứng đọc lại đúng.
3. Viết một hàm template `template <typename... Pins> void set_all_output()` nhận nhiều kiểu `GpioPin` (parameter pack) và gọi `set_mode(Mode::Output)` cho tất cả — đây là "cách vòng" đã nhắc ở bài tập chương 15 để làm việc với nhiều kiểu GPIO khác nhau mà không cần lớp cơ sở chung.
4. So sánh assembly sinh ra giữa `LedPin::write(true)` và một hàm `gpio_write(GpioRegs*, unsigned, bool)` tương đương nhận tham số runtime, cùng cờ `-O2` — chênh lệch (nếu có) đến từ đâu?

## Tóm tắt
- Template tham số không phải kiểu biến `(port, pin)` thành một phần của kiểu: nhầm chân trở thành lỗi biên dịch thay vì lỗi runtime.
- Toàn bộ trạng thái là thanh ghi phần cứng; các hàm static không tốn RAM cho "thể hiện" và thường được inline hoàn toàn.
- Đánh đổi: mất khả năng chọn chân lúc chạy hoặc gộp nhiều chân vào một mảng đồng nhất — chọn cách phù hợp theo tiêu chí đã có ở chương 15.
