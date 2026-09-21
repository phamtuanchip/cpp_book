---
chapter: 10
title: "Mảng, chuỗi và std::span"
part: 2
code: code/chapter-10
---

# Chương 10. Mảng, chuỗi và std::span

## Mục tiêu
- Dùng `std::array` thay mảng thô khi độ dài biết trước, và `std::span` khi cần một "cửa sổ" nhìn vào dữ liệu.
- Viết bảng tra và thuật toán CRC tính sẵn lúc biên dịch.
- Xây một chuỗi ký tự dung lượng cố định, không cấp phát động, thay cho `std::string` trên MCU nhỏ.

## Câu chuyện: `std::string` trên vi điều khiển 8 KB RAM
Một lập trình viên quen lập trình ứng dụng chuyển sang viết firmware, dùng `std::string` để ghép log. Chương trình chạy được vài giờ rồi treo. Nguyên nhân: `std::string` cấp phát động; việc ghép chuỗi liên tục làm **phân mảnh heap** trên một vùng nhớ chỉ vài KB, tới lúc không còn khối nào đủ lớn thì cấp phát thất bại — và mặc định `std::string`/`new` khi thất bại sẽ ném ngoại lệ hoặc gọi `std::terminate`, thường bị tắt bằng `-fno-exceptions` nên chương trình dừng đột ngột. Giải pháp không phải "không dùng C++" mà là chọn đúng công cụ: chuỗi dung lượng cố định.

## Kiến thức

### 1. `std::array<T, N>`: mảng có kiểu, biết độ dài
So với mảng thô `T arr[N]`, `std::array` không tốn thêm byte nào nhưng có `size()`, hoạt động với thuật toán chuẩn, và **không suy biến thành con trỏ** khi truyền vào hàm — nên hàm nhận đúng `std::array<T, N>&` biết chính xác N, còn mảng thô truyền vào hàm suy biến thành `T*`, mất thông tin độ dài.

### 2. `std::span<T>`: cửa sổ nhìn vào dữ liệu
`std::span` (C++20) là một cặp (con trỏ, độ dài) — không sở hữu, không sao chép dữ liệu. Nó thay thế mẫu `(const uint8_t* p, size_t n)` bằng **một** tham số mang cả hai, và còn nhận được từ mảng thô, `std::array`, hoặc một phần của chúng.

### 3. CRC-8 với bảng tra sinh lúc biên dịch, nhận `std::span`

{{code:chapter-10/crc8_span.cpp}}

{{out:chapter-10/crc8_span}}

Ba điều đáng chú ý:
- `make_crc8_table()` là hàm `constexpr`: bảng 256 phần tử được tính **lúc biên dịch**, nằm trong Flash, không tốn chu kỳ CPU lúc chạy.
- `static_assert(crc8(kCheck) == 0xF4, ...)`: giá trị kiểm tra chuẩn của thuật toán CRC-8/SMBUS (đa thức 0x07, khởi tạo 0) cho chuỗi `"123456789"` được xác nhận **ngay lúc biên dịch**. Nếu ai đó sửa sai bảng hoặc thuật toán, **build sẽ lỗi** trước khi chạy thử.
- `crc8()` nhận `std::span<const std::uint8_t>`: gọi được với mảng thô (`crc8(frame)`, nhờ suy luận kiểu tự động dựng span) hoặc với `subspan()` để tính CRC trên một phần dữ liệu mà không sao chép.

> Ghi chú về nguồn: giá trị kiểm tra `0xF4` cho `"123456789"` là giá trị chuẩn thường dùng để xác nhận cách cài đặt CRC-8/SMBUS (đa thức 0x07); kiểm chứng lại với bảng CRC tham chiếu nếu bạn dùng biến thể khác (đa thức, giá trị khởi tạo, đảo bit khác nhau cho ra kết quả khác).

### 4. Chuỗi dung lượng cố định

{{code:chapter-10/static_string.cpp}}

{{out:chapter-10/static_string}}

Thiết kế đáng học:
- Bộ nhớ nằm **bên trong đối tượng** (`char buf_[Capacity + 1]`), không có `new`/heap; dung lượng cố định lúc biên dịch qua tham số template.
- `format()` dùng `std::snprintf` — hàm C an toàn với bộ đệm, luôn dừng đúng chỗ, trả về độ dài *sẽ* cần (có thể lớn hơn bộ đệm) để phát hiện bị cắt.
- Trả `std::string_view` để đọc dữ liệu **không sao chép**; `string_view` chỉ là (con trỏ, độ dài) như `span`, nhưng dành riêng cho ký tự và có các hàm tiện lợi (`substr`...).
- `StaticString<10>` với nội dung dài hơn bị cắt và `format()` trả `false` — người gọi **biết** để xử lý, thay vì âm thầm mất dữ liệu.

## Ví dụ thực tiễn: gộp bộ đệm nhận UART thành khung
Một bộ nhận UART tích luỹ byte vào `std::array<uint8_t, 64> buf` cùng biến đếm `len`; khi đủ khung (theo `packet_parse` ở chương 9) mới xử lý, rồi dùng `std::span` để đưa `buf` (chỉ phần đã nhận) cho hàm tính CRC — không sao chép, không cấp phát, kích thước tối đa cố định ngay từ lúc khai báo.

## Góc nhúng
- `std::array<T, N>` có kích thước đúng bằng `N * sizeof(T)`, không có chi phí ẩn — kiểm chứng bằng `sizeof`.
- `std::span` là hai từ máy (con trỏ + kích thước `size_t`), thường truyền qua thanh ghi, không cấp phát.
- Chọn `Capacity` của `StaticString` đủ dùng cho trường hợp xấu nhất bạn có thể liệt kê (ví dụ độ dài tối đa của một dòng log), không đoán chừng.
- Bảng CRC 256 phần tử tốn 256 byte Flash cố định; nếu Flash rất hạn hẹp, có cách tính CRC theo bit không cần bảng (chậm hơn) — đánh đổi tốc độ lấy dung lượng.

## Lỗi thường gặp
- Dùng `std::vector`/`std::string` mặc định trên MCU nhỏ mà không kiểm soát heap.
- Truyền mảng thô vào hàm và dùng `sizeof(arr)/sizeof(arr[0])` sau khi nó đã suy biến thành con trỏ trong tham số hàm (`sizeof` lúc đó trả về kích thước con trỏ, không phải mảng) — dùng `std::span`/`std::array` để tránh hẳn lớp lỗi này.
- Quên rằng `std::string_view`/`std::span` **không sở hữu** dữ liệu: nếu buffer gốc bị ghi đè hoặc hết phạm vi, view/span trỏ tới dữ liệu không còn hợp lệ.
- Không kiểm tra giá trị trả về của `format()`/`snprintf`, bỏ sót trường hợp bị cắt.

## Bài tập
1. Đổi đa thức CRC trong `make_crc8_table` (ví dụ 0x1D thay vì 0x07) và tìm một chuỗi kiểm tra khác cho biến thể đó (đọc tài liệu CRC bạn định dùng); cập nhật `static_assert`.
2. Viết `crc16` (CCITT, đa thức 0x1021) theo cùng khuôn: bảng `constexpr`, hàm nhận `std::span<const std::uint8_t>`.
3. Thêm hàm `append()` vào `StaticString` để nối thêm nội dung thay vì ghi đè; xử lý trường hợp không đủ chỗ.
4. Viết hàm `std::size_t count_non_zero(std::span<const std::uint8_t> data)` và gọi nó với một `std::array`, một mảng thô, và một `subspan`.
5. Giải thích bằng ví dụ cụ thể vì sao trả về `std::string_view` trỏ vào một `StaticString` **cục bộ trong hàm** là lỗi; sửa lại cho đúng.

## Tóm tắt
- `std::array` thay mảng thô khi độ dài cố định biết trước: không tốn thêm gì, an toàn kiểu hơn.
- `std::span` gộp (con trỏ, độ dài) thành một tham số, nhận từ nhiều nguồn, không sao chép.
- Bảng tra và các bất biến thuật toán nên là `constexpr` + `static_assert`, kiểm chứng ngay lúc biên dịch.
- Chuỗi dung lượng cố định (template hoá theo `Capacity`) thay `std::string` khi cần tránh cấp phát động.
- `string_view`/`span` không sở hữu dữ liệu — chú ý vòng đời của dữ liệu gốc.
