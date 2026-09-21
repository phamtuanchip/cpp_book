---
chapter: 7
title: "Kiểu dữ liệu, biến và số học an toàn"
part: 2
code: code/chapter-07
---

# Chương 7. Kiểu dữ liệu, biến và số học an toàn

## Mục tiêu
- Dùng đúng kiểu độ rộng cố định (`std::uint8_t`, `std::int32_t`...) thay cho `int`, `long`.
- Hiểu vì sao phép tính trung gian tràn số và cách tránh.
- Xử lý bộ đếm thời gian tràn (sau 49,7 ngày) đúng cách.
- Biết khi nào dùng số nguyên, khi nào số thực, và số điểm cố định là gì.

## Câu chuyện: nhiệt độ âm 3 mV
Một lập trình viên chuyển giá trị ADC 12 bit sang milivolt bằng công thức `raw * 3300 / 4095`. Trên PC thử vài giá trị: đúng. Trên chip 16 bit (hoặc khi lập trình viên ép kiểu về 16 bit giữa chừng), giá trị lớn cho kết quả vô lý, dữ liệu gửi lên đám mây sai suốt hai tuần mới ai phát hiện. Lỗi nằm ở **kích thước kiểu và thứ tự thu hẹp**, không phải công thức.

## Kiến thức

### 1. `int` không có kích thước cố định
Chuẩn C++ chỉ đảm bảo `int` ≥ 16 bit, `long` ≥ 32 bit. Trên AVR 8 bit `int` thường là 16 bit; trên ARM Cortex-M và PC là 32 bit. Vì vậy **firmware dùng kiểu độ rộng cố định** trong `<cstdint>`:

| Kiểu | Ý nghĩa |
|------|---------|
| `std::uint8_t`, `std::int8_t` | 8 bit không dấu / có dấu |
| `std::uint16_t`, `std::int16_t` | 16 bit |
| `std::uint32_t`, `std::int32_t` | 32 bit |
| `std::uint64_t`, `std::int64_t` | 64 bit (chậm và tốn trên MCU 8/16/32 bit) |
| `std::size_t` | Kích thước/chỉ số (rộng bằng con trỏ) |
| `bool` | Đúng/sai (thường 1 byte) |

{{code:chapter-07/fixed_width.cpp}}

{{out:chapter-07/fixed_width}}

Kết quả trên máy tác giả (Windows 64 bit): `long` là 4 byte, còn trên Linux 64 bit `long` là 8 byte — cùng một mã nguồn, hai kích thước khác nhau. Đó là lý do phải dùng kiểu độ rộng cố định khi định dạng dữ liệu (gói tin, thanh ghi).

**Quy tắc chọn kiểu:**
- Dữ liệu ra ngoài (gói tin, thanh ghi, Flash): kiểu độ rộng cố định.
- Bộ đếm vòng lặp và biến cục bộ trong hàm: có thể dùng `unsigned`/`int` để trình biên dịch chọn kích thước nhanh nhất; hoặc `std::uint_fast8_t`.
- Chỉ số mảng: `std::size_t`.
- Đừng dùng `char` cho số; chỉ dùng cho ký tự văn bản. Dữ liệu nhị phân dùng `std::uint8_t` (hoặc `std::byte`).

### 2. Số có dấu và không dấu: hai bộ luật khác nhau

| | Không dấu (`unsigned`) | Có dấu (`signed`) |
|---|------------------------|-------------------|
| Tràn số | **Xác định**: quay vòng (modulo 2ⁿ) | **Hành vi không xác định** (trình biên dịch được phép giả định "không bao giờ xảy ra") |
| Dùng cho | Bit, thanh ghi, bộ đếm quay vòng, kích thước | Đại lượng có thể âm (nhiệt độ, sai số) |

Hai bẫy điển hình:
- `uint8_t a = 200, b = 100; auto c = a + b;` — `c` là `int` (300), vì trước khi cộng, các kiểu nhỏ hơn `int` được **thăng cấp (integral promotion)** lên `int`. Gán lại vào `uint8_t` mới bị cắt còn 44.
- So sánh số có dấu với không dấu (`-1 < 1u` là **sai**): `-1` bị chuyển sang số không dấu rất lớn. Bật `-Wall -Wextra` để được cảnh báo `-Wsign-compare`.

### 3. Tràn số trung gian
Xem ví dụ chuyển ADC sang mV:

{{code:chapter-07/adc_convert.cpp}}

{{out:chapter-07/adc_convert}}

Cột "sai" thu hẹp tích `raw * 3300` về 16 bit **trước** khi chia; với `raw = 4095` tích là 13 513 500, bị cắt modulo 65 536 còn 13 084, chia 4095 ra 3 thay vì 3300. Bản đúng nhân trong 32 bit, chia xong rồi mới ép về 16 bit. Quy tắc: **thu hẹp kiểu ở bước cuối cùng, và luôn viết `static_cast` tường minh để người đọc thấy nơi có thể mất thông tin**.

### 4. Bộ đếm thời gian tràn
Hầu hết firmware có `millis()` đếm mili-giây bằng số 32 bit không dấu. `2³² ms ≈ 49,7 ngày`; sau đó nó quay về 0. Thiết bị chạy liên tục sẽ đụng cột mốc này. Cách so sánh thời gian **đúng**: dùng **khoảng thời gian trôi qua**, không dùng thời điểm tuyệt đối.

{{code:chapter-07/tick_wrap.cpp}}

{{out:chapter-07/tick_wrap}}

Ở mốc 100 ms, bản "sai" (`now >= deadline`) báo hết giờ ngay vì `deadline` đã quay vòng thành số nhỏ, còn `now` vẫn ở gần đỉnh. Bản đúng `now - start >= timeout` luôn đúng miễn khoảng đo nhỏ hơn 2³¹ ms, nhờ phép trừ không dấu quay vòng. Đây là lỗi kinh điển: chạy tốt vài tuần trong phòng lab, rồi thiết bị ngoài hiện trường bỗng "hết giờ" ngẫu nhiên.

### 5. Số thực: khi nào và giá phải trả
- MCU **không có FPU** (nhiều Cortex-M0/M3, AVR): mọi phép `float` được mô phỏng bằng phần mềm, chậm (hàng chục đến hàng trăm chu kỳ) và kéo thư viện lớn vào Flash.
- MCU **có FPU** đơn (Cortex-M4F/M7): `float` nhanh, nhưng `double` vẫn chậm nếu FPU chỉ hỗ trợ độ chính xác đơn. Chú ý hậu tố: `1.0f` là `float`, `1.0` là `double`.
- **Số điểm cố định (fixed-point)**: lưu số thực dưới dạng số nguyên nhân với một hệ số. Ví dụ nhiệt độ 25,3 °C lưu thành `253` (đơn vị 0,1 °C) — dùng ở ví dụ `Reading` chương 9. Nhanh, xác định, không cần FPU.
- Không so sánh số thực bằng `==`; dùng khoảng sai số.

Cách đặt tên giúp tránh nhầm đơn vị: `temp_c10` (độ C x10), `voltage_mv`, `timeout_ms`. Đây là thói quen rẻ nhất có lợi nhất.

## Ví dụ thực tiễn: bộ lọc trung bình cho cảm biến (số nguyên)
Cảm biến nhiệt độ nhiễu ±2 đơn vị; ta lọc bằng trung bình trượt 8 mẫu bằng số nguyên, không dùng `float`. Trung bình 8 phần tử chia cho 8 = dịch phải 3 bit. Tổng 8 mẫu 12 bit tối đa 8 × 4095 = 32 760 nằm vừa trong `uint16_t`, nhưng để an toàn ta dùng `uint32_t` cho tổng — luôn tính độ rộng cần thiết trước khi chọn kiểu. Bạn sẽ hoàn thiện bộ lọc này ở bài tập.

## Góc nhúng
- Mỗi lần dùng `std::uint64_t` hoặc `double` trên MCU 8/32 bit không FPU, hãy hỏi: có cần thật không? Kiểm tra bằng `size`/Compiler Explorer.
- `const` toàn cục và `constexpr` nằm trong Flash; tránh biến toàn cục có thể sửa nếu không cần (tiết kiệm RAM và tránh lỗi đồng thời).
- Trên AVR, `float` và chuỗi literal mặc định nằm trong RAM; cần thuộc tính `PROGMEM` để đặt trong Flash — kiến trúc Harvard có quy tắc riêng.

## Lỗi thường gặp
- Dùng `int` cho dữ liệu gói tin; đổi sang chip khác là hỏng.
- Ép kiểu bằng kiểu C `(uint8_t)x` — ẩn mất mát thông tin. Dùng `static_cast`.
- So sánh thời gian bằng `now >= deadline` (mục 4).
- Quên hậu tố `u` cho hằng lớn: `1 << 31` là số có dấu tràn; dùng `1u << 31`.
- Chia số nguyên cho 0: kết quả không xác định trên nhiều nền tảng; luôn kiểm tra mẫu số nếu nó đến từ bên ngoài.
- Dịch bit ≥ độ rộng kiểu (`1u << 32`): hành vi không xác định.

## Bài tập
1. Trong `adc_convert.cpp`, thêm hàm chuyển ADC sang nhiệt độ (đơn vị 0,1 °C) theo công thức `temp_c10 = raw * 1000 / 4095 - 400` bằng số nguyên. Chọn kiểu trung gian đúng và `static_assert` hai giá trị biên.
2. Viết `class MovingAverage8` (mảng 8 phần tử `uint16_t`, vòng tròn, tổng `uint32_t`) với hàm `add(uint16_t) -> uint16_t` trả trung bình bằng dịch phải 3.
3. Sửa `tick_wrap.cpp` để `Tick` là `std::uint16_t` (tràn sau 65 giây). Thử với `timeout = 60000` và giải thích khi nào khoảng đo còn đúng.
4. Viết chương trình chứng minh `-1 < 1u` là sai; bật `-Wall -Wextra` và chép cảnh báo.
5. Tính số bit tối thiểu để lưu độ ẩm 0,0–100,0 % với độ phân giải 0,1 %. Chọn kiểu nhỏ nhất.

## Tóm tắt
- Dùng kiểu độ rộng cố định cho dữ liệu ra ngoài; `size_t` cho chỉ số.
- Số không dấu tràn theo modulo (xác định); số có dấu tràn là hành vi không xác định.
- Kiểu nhỏ hơn `int` được thăng cấp trước khi tính; thu hẹp ở bước cuối với `static_cast`.
- So sánh thời gian bằng `now - start`, không bằng thời điểm tuyệt đối.
- Ưu tiên số nguyên/điểm cố định trên MCU không có FPU; đặt tên biến kèm đơn vị.
