---
chapter: 9
title: "Con trỏ, tham chiếu và const đúng chỗ"
part: 2
code: code/chapter-09
---

# Chương 9. Con trỏ, tham chiếu và const đúng chỗ

## Mục tiêu
- Phân biệt bốn tổ hợp `const`/con trỏ và biết dùng cái nào khi nào.
- Thay tham số `(con trỏ, độ dài)` bằng tham chiếu/struct khi có thể, giữ con trỏ khi bắt buộc (thanh ghi, bộ đệm DMA).
- Viết hàm phân tích gói tin **kiểm tra biên trước khi đọc** — kỹ năng phòng lỗi tràn bộ đệm quan trọng nhất của chương.

## Câu chuyện: đọc quá bộ đệm
Một bộ phân tích gói tin UART giả định trường `length` trong gói luôn đúng với dữ liệu thực nhận. Khi đường truyền nhiễu, `length` đọc được là 200 trong khi chỉ nhận 5 byte — hàm đọc tới `payload[199]`, vượt ra ngoài bộ đệm, ghi đè hoặc đọc rác. Đây là lớp lỗi phổ biến nhất trong code C/C++ xử lý dữ liệu từ bên ngoài (cảm biến, mạng, thẻ nhớ). Chương này xây thói quen kiểm tra biên như phản xạ.

## Kiến thức

### 1. Bốn tổ hợp `const` với con trỏ
Đọc từ phải sang trái:

```cpp
int* p;               // con trỏ tới int: đổi được p, đổi được *p
const int* p;         // con trỏ tới const int: đổi được p, KHÔNG đổi được *p
int* const p;         // con trỏ const tới int: KHÔNG đổi được p, đổi được *p
const int* const p;   // cả hai đều không đổi được
```

{{code:chapter-09/const_demo.cpp}}

{{out:chapter-09/const_demo}}

**Quy tắc thực dụng:** tham số hàm nhận dữ liệu chỉ để đọc → `const T*` hoặc `const T&`. Không có lý do gì để thiếu `const` ở tham số bạn không sửa; nó là tài liệu tự động cho người đọc và cho trình biên dịch kiểm tra hộ bạn.

### 2. Con trỏ và tham chiếu: chọn cái nào

| | Con trỏ (`T*`) | Tham chiếu (`T&`) |
|---|-----------------|---------------------|
| Có thể là "rỗng" | Có (`nullptr`) | Không (luôn trỏ tới một đối tượng) |
| Đổi trỏ sang chỗ khác sau khi gán | Có | Không |
| Cú pháp tại nơi gọi | `f(&x)` | `f(x)` |
| Dùng cho | Bộ nhớ ánh xạ thanh ghi, mảng kiểu C, "có thể không có" | Tham số vào/ra không thể thiếu, tránh sao chép |

Quy tắc: **mặc định dùng tham chiếu**; chỉ dùng con trỏ khi giá trị có thể vắng mặt (`nullptr`) hoặc khi làm việc trực tiếp với địa chỉ phần cứng.

### 3. Tham số ra (output parameter)
C++ không có "trả về nhiều giá trị" gọn như một số ngôn ngữ khác (trước C++17), nhưng có ba cách hay dùng trong nhúng:

```cpp
// Cách 1: tham chiếu ra + trạng thái trả về (dùng khi có thể thất bại)
[[nodiscard]] bool read_sensor(std::uint16_t raw, Reading& out);

// Cách 2: trả về struct theo giá trị (rõ ràng, không có nghĩa "lỗi")
Reading make_reading(std::uint16_t raw);

// Cách 3 (C++17): cấu trúc chia (structured bindings) với std::optional — xem chương 16
```

Ví dụ đọc cảm biến với tham số ra:

{{code:chapter-09/const_demo.cpp}}

`read_sensor` trả `bool` cho biết cảm biến có phản hồi không, và **ghi kết quả** vào `out` chỉ khi thành công — người gọi không phải đoán giá trị nào là "lỗi".

### 4. Kiểm tra biên trước khi đọc: phân tích gói tin

{{code:chapter-09/packet_parse.cpp}}

{{out:chapter-09/packet_parse}}

Thứ tự kiểm tra là **cố ý**:
1. Con trỏ khác `nullptr` và đủ độ dài tối thiểu (start + len + checksum).
2. Byte bắt đầu đúng.
3. `len` đọc từ gói **chưa được tin**; kiểm tra `size` có đủ chỗ chứa `len` byte payload **trước khi** đọc bất kỳ byte payload nào.
4. Chỉ sau khi biên an toàn mới tính checksum.

Đổi thứ tự bước 3 và việc đọc payload là chính xác loại lỗi "đọc quá bộ đệm" nêu ở đầu chương. Ca thử "bị cắt" (`cut`) minh hoạ: gói khai `len = 5` nhưng bộ đệm chỉ có 3 byte — hàm trả `BadLength` thay vì đọc rác.

## Ví dụ thực tiễn: vì sao không dùng `strlen`/con trỏ null-terminated cho dữ liệu nhị phân
Dữ liệu cảm biến, gói tin mạng có thể chứa byte `0x00` hợp lệ ở giữa. Nếu bạn biểu diễn bằng "chuỗi kết thúc bằng 0" (`const char*`) và dùng `strlen`, độ dài bị cắt ngắn sai. Luôn mang **độ dài đi kèm dữ liệu** (như `Packet::len` ở trên, hoặc `std::span` ở chương 10) cho dữ liệu nhị phân; chỉ dùng chuỗi kết thúc bằng 0 cho văn bản thật sự.

## Góc nhúng
- Con trỏ tới thanh ghi phần cứng luôn là `volatile T*` (không phải chỉ `T*`) — chương 21 giải thích vì sao.
- Truyền struct nhỏ (≤ 2 từ máy, ví dụ `Reading` 4 byte) theo giá trị có thể rẻ hơn truyền theo tham chiếu (tránh một lần lấy địa chỉ); struct lớn hơn nên truyền `const&`. Không đoán — đo bằng Compiler Explorer nếu nghi ngờ.
- `nullptr` (C++11) có kiểu riêng (`std::nullptr_t`), an toàn hơn macro `NULL` (thường là `0`) khi nạp chồng hàm.

## Lỗi thường gặp
- Tin `length` trong dữ liệu từ bên ngoài mà không kiểm tra so với kích thước bộ đệm thật.
- Trả về con trỏ/tham chiếu tới biến cục bộ đã hết phạm vi (dangling reference) — trình biên dịch thường cảnh báo (`-Wreturn-local-addr`), đừng tắt cảnh báo này.
- Thiếu `const` ở tham số chỉ đọc, khiến người đọc code không chắc hàm có sửa dữ liệu không.
- So sánh con trỏ với `NULL` kiểu C trong ngữ cảnh nạp chồng, gây mơ hồ; dùng `nullptr`.
- Quên rằng con trỏ có thể là `nullptr`: gọi `read_sensor(raw, *ptr)` khi `ptr` có thể rỗng.

## Bài tập
1. Trong `packet_parse.cpp`, thêm ca thử gói có `len = 0` (chỉ có checksum của byte start+len). Kết quả mong đợi là gì? Kiểm tra code có xử lý đúng không.
2. Viết hàm `bool find_byte(const std::uint8_t* buf, std::size_t size, std::uint8_t target, std::size_t& index_out)` trả vị trí byte đầu tiên khớp; dùng tham chiếu ra đúng cách.
3. Giải thích vì sao `const Reading&` tốt hơn `Reading` (theo giá trị) khi `Reading` có thêm một mảng 64 byte bên trong.
4. Cố tình đổi thứ tự kiểm tra ở `parse()`: kiểm tra checksum trước khi kiểm tra `len` so với `size`. Viết một bộ dữ liệu khiến bản này đọc ra ngoài biên (không cần chạy thật, chỉ cần chỉ ra chỉ số bị vi phạm).
5. Viết `struct Span { const std::uint8_t* data; std::size_t size; };` và hàm `checksum(Span s)`; so sánh với cách truyền `(ptr, size)` hai tham số riêng — vì sao gộp lại an toàn hơn?

## Tóm tắt
- Bốn tổ hợp `const`+con trỏ mã hoá rõ ý định: ai được đổi trỏ, ai được đổi giá trị.
- Ưu tiên tham chiếu; dùng con trỏ khi giá trị có thể vắng mặt hoặc khi thao tác địa chỉ phần cứng.
- Tham số ra qua tham chiếu, kèm trạng thái trả về `[[nodiscard]]`, là mẫu an toàn cho hàm có thể thất bại.
- Với dữ liệu từ bên ngoài: luôn kiểm tra độ dài so với bộ đệm thật **trước khi** đọc, không tin trường độ dài trong chính dữ liệu.
- Mang độ dài đi kèm con trỏ (struct hoặc `std::span` — chương sau) thay vì tách rời hai tham số.
