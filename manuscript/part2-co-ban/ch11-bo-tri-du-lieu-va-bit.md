---
chapter: 11
title: "Bố trí dữ liệu, thao tác bit và bit_cast"
part: 2
code: code/chapter-11
---

# Chương 11. Bố trí dữ liệu, thao tác bit và bit_cast

## Mục tiêu
- Hiểu căn lề (alignment) và đệm (padding) trong struct, và cách sắp trường để giảm kích thước.
- Thao tác trường bit của thanh ghi bằng hằng số và hàm `constexpr` thay vì bit-field khó đoán.
- Đổi qua lại giữa số và mảng byte một cách an toàn bằng `std::bit_cast`, hiểu về endianness.

## Câu chuyện: gói tin lớn hơn dự kiến
Một đội định nghĩa gói tin gồm `uint8_t start; uint32_t value; uint8_t crc;` và tính "chắc chắn" kích thước là 6 byte. Khi `sizeof(Packet)` in ra 12, cả đội mất nửa ngày tìm bug ở chỗ khác trước khi nhận ra: **đệm căn lề** đã thêm 3 byte trước `value` và 3 byte ở cuối. Gói tin gửi qua mạng theo `sizeof` sai định dạng ở đầu nhận. Đây là lỗi cực kỳ phổ biến khi struct C++ được dùng trực tiếp làm định dạng trao đổi.

## Kiến thức

### 1. Vì sao có đệm (padding)
Hầu hết CPU đọc/ghi bộ nhớ nhanh nhất khi địa chỉ **chia hết cho kích thước kiểu** (căn lề tự nhiên): `uint32_t` muốn nằm ở địa chỉ chia hết cho 4. Trình biên dịch chèn byte đệm để mọi trường đạt căn lề đó, và đệm ở cuối struct để mảng các struct đó cũng thẳng hàng.

{{code:chapter-11/struct_layout.cpp}}

{{out:chapter-11/struct_layout}}

`Bad` (thứ tự `uint8_t, uint32_t, uint8_t`) tốn 12 byte: 3 byte đệm trước `b` để `b` căn lề 4, cộng 3 byte đệm cuối để kích thước struct là bội của 4 (căn lề lớn nhất trong struct). `Good` (đặt trường lớn trước) chỉ tốn 8 byte. **Quy tắc: sắp trường theo kích thước giảm dần** để giảm đệm.

`Header` minh hoạ cách **khẳng định bố cục bằng `static_assert`**: nếu ai đó sau này thêm một trường làm lệch offset, build báo lỗi ngay thay vì để lỗi lộ ra khi hai thiết bị nói chuyện với nhau bằng định dạng khác nhau.

### 2. Đóng gói chặt: `#pragma pack` — dùng thận trọng
Có thể ép không đệm bằng `#pragma pack(push, 1)` ... `#pragma pack(pop)`, nhưng cái giá là truy cập trường không căn lề có thể **chậm hơn** hoặc trên một số kiến trúc **gây lỗi phần cứng** (bus fault) nếu CPU không hỗ trợ truy cập không căn lề. Với gói tin trao đổi qua mạng, cách an toàn hơn là **serialize thủ công từng byte** (đọc/ghi từng trường vào mảng `uint8_t`, kiểm soát hoàn toàn thứ tự byte) thay vì ép layout của struct — chương 22 (giao thức) quay lại chủ đề này.

### 3. Thao tác bit của thanh ghi

{{code:chapter-11/register_bits.cpp}}

{{out:chapter-11/register_bits}}

So với bit-field (`struct { uint32_t enable : 1; uint32_t mode : 2; };`), cách dùng hằng số + hàm `constexpr` có ưu điểm: **thứ tự bit và cách đóng gói của bit-field không được chuẩn C++ quy định chặt chẽ** (phụ thuộc trình biên dịch/ABI), trong khi phép dịch bit và mặt nạ (mask) tường minh, hoạt động giống hệt nhau trên mọi trình biên dịch — quan trọng khi mô tả thanh ghi phần cứng phải khớp chính xác với datasheet.

`static_assert(set_mode(0xFFFFFFFFu, 0) == (0xFFFFFFFFu & ~ctrl::kModeMask))` xác nhận **chỉ đúng trường `mode` bị xoá**, các bit khác giữ nguyên — đây là lỗi thường gặp khi viết mặt nạ tay (quên đảo bit `~`, hoặc lệch `kModeShift`).

### 4. `std::bit_cast`: đổi biểu diễn an toàn (C++20)
Trước C++20, cách phổ biến để "nhìn" bit của một `float` là ép kiểu con trỏ (`*(uint32_t*)&f`) hoặc dùng `union` — cả hai đều vi phạm hoặc nằm ở vùng xám của **quy tắc alias nghiêm ngặt (strict aliasing)**, khiến trình biên dịch tối ưu có thể sinh mã sai trong một số trường hợp. `std::bit_cast<To>(from)` làm đúng việc đó một cách hợp lệ, với điều kiện `sizeof(To) == sizeof(From)` và cả hai đều là kiểu "trivially copyable".

{{code:chapter-11/bit_cast_endian.cpp}}

{{out:chapter-11/bit_cast_endian}}

**Endianness (thứ tự byte):** hầu hết MCU và PC hiện đại là **little-endian** (byte thấp nhất nằm ở địa chỉ thấp nhất) — kết quả `byte[0]=0x44` (phần thấp của `0x11223344`) xác nhận điều đó trên máy chạy ví dụ này. Một số kiến trúc mạng và một số MCU cũ dùng big-endian. Khi đóng gói dữ liệu để gửi qua mạng hoặc lưu file, giao thức thường quy định rõ thứ tự byte (ví dụ "network byte order" = big-endian) — bạn phải **tự chuyển đổi tường minh** (dịch bit và OR từng byte) thay vì dựa vào endianness của CPU đang chạy code.

## Ví dụ thực tiễn: đóng gói cảm biến thành khung gửi đi
Kết hợp ba kỹ thuật của chương: dùng `Header` không đệm (mục 1) làm phần đầu khung, dùng hằng/hàm bit (mục 3) nếu khung có cờ trạng thái đóng gói trong 1 byte, và khi cần gửi một giá trị `float` qua UART, **không** gửi thẳng `sizeof(float)` byte của biểu diễn nội bộ (phụ thuộc kiến trúc) — thay vào đó nhân với hệ số để chuyển sang số nguyên điểm cố định (chương 7) trước khi đóng gói, hoặc nếu bắt buộc gửi nguyên dạng IEEE-754, dùng `std::bit_cast` sang `uint32_t` rồi tự tách 4 byte theo thứ tự đã thống nhất với đầu nhận.

## Góc nhúng
- Luôn `static_assert(sizeof(...) == N)` cho mọi struct dùng làm định dạng trao đổi (gói tin, bản ghi Flash) — bắt lỗi layout ngay lúc biên dịch, trên mọi máy build.
- Kiểm tra căn lề bằng `alignof(T)`; struct chứa `double` (8 byte) có thể cần căn lề 8 dù chạy trên MCU 32 bit.
- Dùng `-Wpadded` (GCC/Clang) để trình biên dịch tự cảnh báo chỗ nào bị chèn đệm.

## Lỗi thường gặp
- Gán `sizeof(struct)` bằng tổng `sizeof` từng trường mà không tính đệm.
- Ép kiểu con trỏ (`reinterpret_cast`) để "nhìn" bit thay vì `std::bit_cast`, dựa vào hành vi không được đảm bảo bởi chuẩn.
- Gửi struct nguyên khối qua mạng/Flash mà không kiểm soát padding và endianness ở cả hai đầu.
- Viết mặt nạ bit tay, quên `~` khi xoá trường, làm hỏng các bit lân cận.
- Dùng bit-field cho thanh ghi phần cứng rồi ngạc nhiên khi đổi trình biên dịch/kiến trúc cho kết quả khác.

## Bài tập
1. Sắp lại trường của một struct bất kỳ trong dự án của bạn theo kích thước giảm dần; đo `sizeof` trước và sau.
2. Thêm trường `uint8_t flags` vào `Header` sao cho `sizeof(Header)` vẫn là 4 mà không cần đệm; viết `static_assert` xác nhận.
3. Viết hàm `constexpr uint32_t set_field(uint32_t reg, uint32_t mask, unsigned shift, uint32_t value)` tổng quát hoá `set_mode`, dùng lại cho nhiều trường bit khác nhau.
4. Dùng `std::bit_cast` viết hàm `to_be_bytes(uint32_t v) -> std::array<uint8_t, 4>` trả về big-endian bất kể CPU chạy là little hay big-endian (gợi ý: tự dịch bit và gán từng phần tử, không dựa vào `bit_cast` để đổi thứ tự).
5. Giải thích bằng lời (không cần code) vì sao gửi thẳng byte của `float` qua hai thiết bị khác kiến trúc có thể cho kết quả sai, và nêu hai cách tránh.

## Tóm tắt
- Đệm căn lề có thể làm struct lớn hơn tổng các trường; sắp trường theo kích thước giảm dần để giảm đệm.
- Khẳng định layout bằng `static_assert(sizeof...)`/`offsetof` cho mọi struct dùng làm định dạng trao đổi.
- Thao tác bit thanh ghi bằng hằng số và hàm `constexpr` thay vì bit-field để tránh phụ thuộc trình biên dịch.
- `std::bit_cast` là cách hợp lệ theo chuẩn để đổi biểu diễn bit, thay cho ép kiểu con trỏ hoặc `union`.
- Endianness không được đảm bảo giống nhau giữa các thiết bị; đóng gói dữ liệu trao đổi phải tường minh về thứ tự byte.
