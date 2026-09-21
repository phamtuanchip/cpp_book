---
chapter: 3
title: "Chi phí của trừu tượng"
part: 0
code: code/chapter-03
---

# Chương 3. Chi phí của trừu tượng

## Mục tiêu
- Biết bộ nhớ của MCU chia thành Flash, SRAM và cái gì nằm ở đâu.
- Phân loại tính năng C++ theo: miễn phí / tốn Flash / tốn RAM / tốn CPU.
- Biết cách **tự đo** thay vì tin lời đồn.

## Câu chuyện: 2 KB RAM
Bo Arduino Uno dùng vi điều khiển ATmega328P với 32 KB Flash và 2 KB SRAM. Lập trình bằng... C++ (trình biên dịch avr-gcc). Nghĩa là C++ chạy được trong 2 KB RAM — nếu bạn biết mình đang trả gì. Chương này là bản đồ chi phí.

## Kiến thức

### Bộ nhớ của MCU

| Vùng | Thường là | Chứa gì |
|------|-----------|---------|
| Flash | Bộ nhớ không mất khi tắt nguồn | Mã lệnh (`.text`), hằng số (`.rodata`), giá trị khởi tạo của biến toàn cục |
| SRAM | Bộ nhớ chạy | Biến toàn cục (`.data`, `.bss`), stack, heap |

Một biến `const` toàn cục thường nằm trong Flash và **không** chiếm RAM — nhưng chỉ khi trình biên dịch/linker được phép làm vậy (ví dụ dùng `constexpr`, `const`; trên AVR còn cần thêm thuộc tính riêng vì kiến trúc Harvard).

### Bảng chi phí tính năng

| Tính năng | Flash | RAM | CPU lúc chạy | Ghi chú |
|-----------|-------|-----|--------------|---------|
| `class`, `namespace`, `enum class` | 0 | 0 | 0 | Chỉ là tổ chức lúc biên dịch |
| `template` | Có thể phình nếu tạo nhiều bản | 0 | 0 | Mỗi bộ tham số sinh một bản code |
| `constexpr` / `static_assert` | Giảm | Giảm | Giảm | Chuyển tính toán sang lúc biên dịch |
| Hàm `inline` / RAII | ≈0 | 0 | ≈0 | Thường được tối ưu như code C tay |
| Hàm ảo | Bảng vtable mỗi lớp | +1 con trỏ mỗi đối tượng | Gọi gián tiếp | Xem ví dụ dưới |
| Ngoại lệ (`throw`) | **Tăng đáng kể** (bảng unwind) | Có thể có | Đường lỗi chậm, khó dự đoán | Thường tắt bằng `-fno-exceptions` |
| RTTI | Tăng (typeinfo) | 0 | `dynamic_cast` chậm | Tắt bằng `-fno-rtti` |
| `new`/`delete`, `std::vector` | Kéo theo heap | Phân mảnh | Thời gian không xác định | Tránh trong vòng lặp thời gian thực |
| `iostream` | **Rất lớn** | Lớn | Chậm | Dùng `printf` hoặc UART tự viết |

Cột "Flash/RAM" là *xu hướng*; con số cụ thể phụ thuộc trình biên dịch, cờ tối ưu và chip. Bạn đo được — xem phần "Góc nhúng".

## Ví dụ 1: trừu tượng nhưng không tốn phí

{{code:chapter-03/zero_overhead.cpp}}

{{out:chapter-03/zero_overhead}}

`sum_cpp(kData)` được tính ngay lúc biên dịch nhờ `constexpr` và `static_assert` xác nhận kết quả. Kiểu `std::array<uint8_t, 4>` mang theo độ dài, nên không thể truyền sai như `sum_c(raw, 40)`.

## Ví dụ 2: hàm ảo có giá bao nhiêu

{{code:chapter-03/vtable_cost.cpp}}

{{out:chapter-03/vtable_cost}}

Trên máy tác giả (GCC 16.2, Windows 64-bit) kết quả là 4 và 16 byte: đối tượng có hàm ảo chứa thêm một con trỏ vptr (8 byte) và 4 byte đệm để căn lề, nên lớn gấp bốn lần dù chỉ có một `int`. Kết quả trên máy bạn có thể khác nếu kiến trúc khác. Trên MCU 32-bit con trỏ chiếm 4 byte; trên PC 64-bit là 8 byte. Nếu bạn có 500 đối tượng nhỏ, phần vptr đã lên tới 2 KB.

## Góc nhúng: cách tự đo

1. **Kích thước code/RAM tổng:**
   ```bash
   arm-none-eabi-size firmware.elf
   ```
   Cột `text` ≈ Flash, `data` + `bss` ≈ RAM tĩnh.
2. **Ai chiếm bao nhiêu:** `arm-none-eabi-nm --size-sort -S firmware.elf | tail`.
3. **So sánh hai cách viết** trên https://godbolt.org với cờ `-Os` hoặc `-O2`.
4. **Bản đồ liên kết:** thêm `-Wl,-Map=firmware.map` và đọc phần `.text` theo file đối tượng.
5. **Tắt tính năng đắt:** thêm `-fno-exceptions -fno-rtti` (và kiểm tra thư viện của bạn còn dùng được không).

> Quy tắc: không nói "C++ tốn X byte". Nói "trong build của tôi, tính năng này thêm X byte" và cho lệnh đo.

## Lỗi thường gặp
- Bật `-O0` khi đo kích thước rồi kết luận "C++ phình". Đo với `-Os`/`-O2`.
- Đưa `<iostream>` vào firmware chỉ để in log.
- Đặt đối tượng lớn có hàm ảo thành mảng toàn cục mà không tính vptr.
- Quên rằng template tạo bản code riêng cho mỗi tham số (`Gpio<1>`, `Gpio<2>`, ...): nhỏ với hàm inline, đáng kể với hàm lớn.

## Bài tập
1. Chạy `vtable_cost.cpp`, thêm một hàm ảo nữa. `sizeof` có đổi không? Giải thích.
2. Thay `std::array` bằng mảng thô trong `sum_cpp`. Còn `static_assert` được không? Vì sao?
3. Đưa `sum_c` và `sum_cpp` vào Compiler Explorer (ARM GCC, `-O2`). So sánh assembly của hai hàm.
4. Liệt kê những tính năng ở bảng chi phí mà dự án nhúng hiện tại của bạn đang dùng.

## Tóm tắt
- Trừu tượng lúc biên dịch (class, template, constexpr) thường không tốn phí lúc chạy.
- Ngoại lệ, RTTI, heap, iostream là những nơi phí thật sự nằm.
- Hàm ảo tốn một con trỏ mỗi đối tượng và một lần gọi gián tiếp.
- Mọi kết luận về chi phí phải đi kèm phép đo.
