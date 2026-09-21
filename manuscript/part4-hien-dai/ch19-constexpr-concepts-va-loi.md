---
chapter: 19
title: "constexpr, concepts và xử lý lỗi không exception"
part: 4
code: code/chapter-19
---

# Chương 19. `constexpr`, concepts và xử lý lỗi không exception

## Mục tiêu
- Chuyển việc tính bảng tra và kiểm tra cấu hình sang **lúc biên dịch** bằng `constexpr`, `static_assert`, `if constexpr`.
- Biết `consteval`, `constinit` và khi nào cần chúng.
- Thiết kế xử lý lỗi cho build `-fno-exceptions`: `Result<T>`/`std::expected` thay cho exception.
- Tổng kết cách chọn công cụ C++ hiện đại cho firmware.

## Câu chuyện: bảng CRC 256 phần tử chiếm 256 byte RAM
Một bản firmware tính bảng CRC-8 khi khởi động: vòng lặp 256×8 lần, tốn thời gian boot và 256 byte RAM. Bảng này là hằng số — không có lý do gì để tính lúc chạy. Với `constexpr`, trình biên dịch tính sẵn và đặt kết quả vào Flash (`.rodata`); `static_assert` còn kiểm tra luôn với vector chuẩn ngay lúc biên dịch.

## Kiến thức

### 1. `constexpr` và `static_assert`

{{code:chapter-19/constexpr_crc.cpp}}

{{out:chapter-19/constexpr_crc}}

- `make_crc8_table` là hàm `constexpr` thường: có vòng lặp, biến cục bộ, `std::array` — từ C++14/17 đều hợp lệ. `inline constexpr auto kCrcTable = ...` buộc gọi nó **lúc biên dịch**.
- `static_assert(crc8(...) == 0xF4)` dùng vector chuẩn CRC-8 (poly 0x07, dữ liệu `"123456789"`, kết quả 0xF4). Nếu sai đa thức hay thuật toán, **code không biên dịch** — kiểm thử miễn phí, chạy mọi lần build, không cần phần cứng.
- Cùng hàm `crc8` được gọi ở lúc chạy với dữ liệu thật (`msg`). Đó là điểm mạnh: **một** code cho cả hai thời điểm.
- Bảng nằm ở đâu? Với `constexpr` toàn cục thường vào `.rodata` (Flash). Xác nhận bằng file `.map` hoặc `arm-none-eabi-size` — đừng đoán.

### 2. `if constexpr`
Trong template, `if constexpr` chọn nhánh **lúc biên dịch** và loại nhánh còn lại khỏi code sinh ra (nhánh bị loại không cần hợp lệ với kiểu đó, miễn là còn phụ thuộc tham số template). Dùng cho: chọn cách ghi thanh ghi 8/16/32-bit, chọn thuật toán theo kích thước kiểu, bỏ code debug.

### 3. `consteval` và `constinit` (C++20)
- `consteval`: hàm **bắt buộc** chạy lúc biên dịch (không thể gọi với giá trị lúc chạy). Hợp với hàm tính hằng như `baud_divisor(clk, baud)` — gọi sai lúc chạy là lỗi.
- `constinit`: biến toàn cục **bắt buộc** khởi tạo tĩnh (không có "static initialization order fiasco" lúc khởi động), nhưng vẫn sửa được lúc chạy. Hữu ích cho trạng thái toàn cục cần sẵn sàng trước `main`.
- `constexpr` = "có thể" tính lúc biên dịch; hai từ khoá trên siết chặt thành "phải".

### 4. Concepts — nhắc lại và mở rộng
Ở chương 15 ta ràng buộc kiểu bằng `concept`. Cùng cú pháp dùng để **ghi lại hợp đồng của driver**:

```cpp
template <typename T>
concept Transport = requires(T t, std::uint8_t b) {
    { t.init() } -> std::same_as<void>;
    { t.write(b) } -> std::same_as<void>;
};

template <Transport T>
void send(T& t, std::uint8_t b) { t.write(b); }
```

Truyền vào kiểu thiếu `write` → lỗi ngay tại điểm gọi, nêu rõ ràng buộc nào không thoả.

### 5. Xử lý lỗi khi không dùng exception

{{code:chapter-19/result_type.cpp}}

{{out:chapter-19/result_type}}

Nhiều firmware build với `-fno-exceptions` (tiết kiệm Flash, thời gian chạy dự đoán được, MISRA/AUTOSAR hạn chế exception). Khi đó lỗi phải đi qua **giá trị trả về**:

| Cách | Ưu | Nhược |
|------|----|-------|
| Mã lỗi `int`/`enum` + tham số ra | Quen thuộc, không overhead | Dễ quên kiểm tra, kết quả đi qua con trỏ |
| `std::optional<T>` | Đơn giản | Không cho biết **vì sao** lỗi |
| `Result<T>` / `std::expected<T, E>` (C++23) | Mang được cả giá trị lẫn lỗi; buộc xử lý cả hai | Cần trình biên dịch mới hoặc tự viết bản tối giản |

`Result<T>` trong ví dụ dùng `std::variant<T, Error>` với `std::get_if` để **không ném exception**. Thêm `[[nodiscard]]` lên kiểu (hoặc hàm) để trình biên dịch cảnh báo khi người gọi bỏ qua kết quả. Nếu toolchain của bạn hỗ trợ C++23, `std::expected` (header `<expected>`) làm đúng việc này — cần kiểm tra bản `arm-none-eabi-g++` bạn dùng có hỗ trợ chưa.

Nguyên tắc: lỗi **dự kiến** (timeout, CRC sai, hết buffer) → trả giá trị lỗi; lỗi **không thể xảy ra** (vi phạm bất biến) → `assert`/dừng hệ thống có kiểm soát.

## Tổng kết Phần 4: chọn công cụ nào?

| Bài toán | Công cụ |
|----------|---------|
| Bảng tra, hằng cấu hình, kiểm tra hợp lệ | `constexpr` + `static_assert` |
| Callback vào API C | Lambda không capture |
| Hành vi tuỳ biến trong thuật toán | Template + lambda |
| Mảng cố định, bảng | `std::array` |
| Giá trị có thể vắng | `std::optional` |
| Tập sự kiện đóng | `std::variant` + `std::visit` |
| Tài nguyên có chủ duy nhất | Handle move-only / `unique_ptr` + deleter |
| Lỗi có lý do, không exception | `Result<T>` / `std::expected` |

## Góc nhúng
- Đo thời gian boot trước/sau khi chuyển bảng CRC sang `constexpr` (đọc bộ đếm chu kỳ DWT hoặc timer) — đó là con số của **bạn**, không phải con số trong sách.
- `-fno-exceptions -fno-rtti` cùng `-Os` là bộ cờ thường gặp cho MCU nhỏ; kiểm tra `std::variant`/`<optional>` vẫn biên dịch được với chúng trên toolchain của bạn.
- Bảng `constexpr` lớn làm tăng Flash: cân nhắc kích thước bảng (256 phần tử) so với tính trực tiếp từng bit khi Flash chật.

## Lỗi thường gặp
- Tưởng `constexpr` luôn chạy lúc biên dịch: nếu đối số là giá trị lúc chạy, hàm vẫn chạy lúc chạy. Muốn ép, gán vào `constexpr` biến hoặc dùng `consteval`.
- `constexpr` biến cục bộ trong hàm không đảm bảo nằm ở Flash; đặt ở phạm vi file hoặc `static`.
- Để `static_assert` với thông báo mơ hồ; viết rõ điều gì sai.
- Trả `Result` nhưng bỏ qua kết quả; thiếu `[[nodiscard]]`.
- Gọi `std::get<T>` trên `variant` trong build không exception.

## Bài tập
1. Đổi đa thức thành 0x1D (CRC-8/SAE-J1850 khác ở giá trị khởi tạo và XOR cuối, nên chỉ đổi bảng là chưa đủ); tra tài liệu chuẩn để lấy vector kiểm thử đúng rồi thêm `static_assert` mới.
2. Viết `consteval std::uint32_t baud_divisor(std::uint32_t clk, std::uint32_t baud)` có `static_assert`-kiểu kiểm tra `baud != 0`; thử gọi với biến lúc chạy và đọc lỗi.
3. Thêm `[[nodiscard]]` vào `Result` và cố tình bỏ qua kết quả `read_adc` — xem cảnh báo.
4. Viết concept `Transport` như mục 4 và áp dụng cho `UartDriver`/`SpiDriver` của chương 15.

## Tóm tắt
- `constexpr` + `static_assert` chuyển tính toán và kiểm thử sang lúc biên dịch: rẻ, sớm, không cần phần cứng.
- `if constexpr`, `consteval`, `constinit` siết chặt "khi nào" mọi thứ xảy ra.
- Không exception ≠ không xử lý lỗi: dùng `Result<T>`/`std::expected` để lỗi có lý do và không bị bỏ sót.
- Phần 4 cho bộ công cụ đủ để viết firmware C++ hiện đại: an toàn hơn C, chi phí đo được, không bắt buộc heap.
