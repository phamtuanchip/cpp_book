---
chapter: 17
title: "std::array, optional, variant và string_view"
part: 4
code: code/chapter-17
---

# Chương 17. `std::array`, `std::optional`, `std::variant` và `std::string_view`

## Mục tiêu
- Thay mảng C thô bằng `std::array` mà không mất gì về RAM/tốc độ.
- Biểu diễn "không có giá trị" bằng `std::optional` thay vì số ma thuật (`-1`, `0xFFFF`).
- Mô hình hoá tập sự kiện đóng bằng `std::variant` + `std::visit`, không vtable, không heap.
- Dùng `std::string_view` để xem chuỗi mà không sao chép.

## Câu chuyện: hàm đọc cảm biến trả `-1` khi lỗi
Một hàm trả `int` với quy ước "âm là lỗi" hoạt động cho tới ngày cảm biến trả giá trị âm hợp lệ (nhiệt độ −5 °C). Kiểu trả về không nói lên "có thể không có giá trị"; người gọi quên kiểm tra và code chạy tiếp với dữ liệu rác. C++ hiện đại cho kiểu để **nói điều đó ra**.

## Kiến thức

### 1. `std::array` và `std::optional`

{{code:chapter-17/array_optional.cpp}}

{{out:chapter-17/array_optional}}

- **`std::array<T, N>`** là mảng C bọc trong struct: kích thước nằm trong kiểu, không phân rã thành con trỏ khi truyền hàm, có `.size()`, `.begin()`, `.at()`; vẫn nằm trên stack/static, **không heap**. Với `-O2`, `operator[]` biên dịch giống hệt mảng thô. Truyền `const std::array<...>&` giữ nguyên thông tin kích thước — khác hẳn `uint16_t*` (chương 10).
- **`std::optional<T>`** = `T` cộng một cờ "có giá trị". `first_above` trả `std::nullopt` khi không tìm thấy, và `if (auto idx = ...)` ép người gọi xử lý hai trường hợp. Chi phí: thêm cờ + căn chỉnh (`sizeof(optional<uint8_t>)` thường 2, `optional<uint32_t>` thường 8 — chương in ra để bạn tự kiểm chứng trên toolchain của mình).
- Không dùng `optional` cho lỗi cần **lý do** (timeout hay CRC sai?) — với trường hợp đó xem `Result<T>` ở chương 19.

### 2. `std::string_view` — nhìn mà không sao chép
`std::string_view` là cặp (con trỏ, độ dài), không sở hữu dữ liệu, không heap. `parse_adc(std::string_view)` nhận được chuỗi literal, mảng `char`, hay một đoạn của buffer UART mà không cần `\0` cuối. Hai bẫy:
- View **không sống lâu hơn** dữ liệu gốc (dangling view giống dangling pointer).
- Không đảm bảo kết thúc bằng `\0`: khi đưa vào API C như `printf("%s")` phải dùng `%.*s` với độ dài (như ví dụ), không phải `data()` trần.

### 3. `std::variant` — sự kiện đóng, kiểm tra đủ nhánh

{{code:chapter-17/variant_event.cpp}}

{{out:chapter-17/variant_event}}

`std::variant<A, B, C>` chứa **đúng một** trong các kiểu đó, lưu ngay tại chỗ (không heap), kích thước ≈ kiểu lớn nhất + chỉ số kiểu. `std::visit` với bộ lambda `Overloaded` gọi nhánh tương ứng; **nếu thiếu một kiểu, code không biên dịch** — thêm loại sự kiện mới thì trình biên dịch chỉ ra mọi nơi cần cập nhật. So với lớp ảo (chương 14):

| | `std::variant` | Lớp ảo |
|---|---|---|
| Tập kiểu | **Đóng**, biết lúc biên dịch | Mở, có thể thêm lớp con |
| Bộ nhớ | Trong đối tượng, không heap | Cần con trỏ/tham chiếu tới đối tượng |
| Thêm thao tác mới | Thêm một `visit`, không sửa các kiểu | Thêm hàm ảo, sửa mọi lớp con |
| Thêm kiểu mới | Sửa mọi `visit` (trình biên dịch báo) | Thêm một lớp con |

Với hàng đợi sự kiện của máy trạng thái (chương sau) `variant` thường là lựa chọn gọn nhất.

Lưu ý build `-fno-exceptions`: `std::get<T>` ném `bad_variant_access` khi sai kiểu; thay bằng `std::get_if` hoặc `std::visit` (chương 19 dùng `get_if`).

## Góc nhúng
- Đo `sizeof(Event)` (in trong ví dụ) và nhân với độ sâu hàng đợi để biết RAM cần — `std::array<Event, N>` làm hàng đợi tĩnh.
- `std::array` rất hợp với bảng hằng đặt vào Flash: khai báo `constexpr`/`static const` để linker đặt vào `.rodata` thay vì RAM (kiểm tra bằng file `.map`).
- Cả bốn kiểu trong chương là header-only, không cần heap; nhưng luôn kiểm tra kích thước Flash sau khi thêm `<variant>`/`<optional>` với `-Os` trên toolchain của bạn.

## Lỗi thường gặp
- Dùng `optional::value()` rồi build không exception: gọi khi rỗng sẽ ném `bad_optional_access` (hoặc abort). Kiểm tra `has_value()` trước, hoặc dùng `*opt` sau khi đã kiểm tra.
- `string_view` trỏ vào buffer đã bị ghi đè hoặc chuỗi tạm đã bị hủy.
- Quên `Overloaded` phải liệt kê **mọi** kiểu; thêm nhánh `[](auto&&){}` để "nuốt hết" sẽ làm mất kiểm tra đủ nhánh.
- Dùng `std::array` rồi vẫn truyền `.data()` + tự tay mất kích thước.

## Bài tập
1. Thêm `struct Fault { std::uint8_t code; };` vào `Event` và sửa `handle`. Quan sát lỗi biên dịch trước khi sửa.
2. Viết `template <std::size_t N> std::optional<std::size_t> find(const std::array<std::uint8_t, N>&, std::uint8_t)`.
3. Viết hàm `split_first(std::string_view s, char sep)` trả về phần trước dấu phân cách bằng `string_view` (không sao chép).
4. So sánh bằng lời `std::variant<A,B,C>` và `union` C + trường `type` viết tay: `variant` chặn được lỗi nào?

## Tóm tắt
- `std::array` = mảng C có kích thước trong kiểu, cùng chi phí; `std::optional` nói rõ "có thể không có"; `std::string_view` xem chuỗi không sao chép.
- `std::variant` + `std::visit` là "union an toàn" cho tập sự kiện đóng, kiểm tra đủ nhánh lúc biên dịch.
- Cả bốn đều không heap; chi phí thật là cờ/chỉ số kiểu và căn chỉnh — hãy đo `sizeof`.
