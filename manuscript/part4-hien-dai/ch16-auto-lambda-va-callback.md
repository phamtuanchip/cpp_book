---
chapter: 16
title: "auto, lambda và callback không tốn heap"
part: 4
code: code/chapter-16
---

# Chương 16. `auto`, lambda và callback không tốn heap

## Mục tiêu
- Dùng `auto`, range-for và structured binding để code ngắn hơn mà không đổi chi phí chạy.
- Viết lambda (không capture / có capture) và hiểu nó biên dịch thành gì.
- Chọn đúng cách truyền callback: con trỏ hàm, template, hay `std::function` — và biết vì sao nhúng thường tránh cái cuối.

## Câu chuyện: callback UART trong firmware
Driver UART cần báo "có byte mới" cho tầng ứng dụng. Trong C ta dùng con trỏ hàm kèm `void* ctx`. Trong C++ hiện đại, người mới hay với tay tới `std::function<void(uint8_t)>` vì nó "tiện": nhận được cả lambda có capture. Nhưng `std::function` dùng type erasure, và với callable lớn có thể cấp phát heap — thứ mà một MCU 64 KB RAM không muốn thấy lúc chạy. Chương này cho bạn hai lựa chọn rẻ hơn.

## Kiến thức

### 1. `auto` — bớt gõ, không bớt kiểm soát
`auto` chỉ là suy luận kiểu lúc **biên dịch**; không có chi phí chạy. Dùng khi kiểu đã rõ từ vế phải (`auto it = ...`, `auto lease = lease_spi(0)`), tránh khi nó che mất kiểu quan trọng — đặc biệt với số nguyên:

```cpp
auto a = 200;            // int, không phải uint8_t
std::uint8_t b = 200;    // rõ ràng: 1 byte
auto c = b + 1;          // int (integer promotion, xem chương 7)
```

Trên MCU, kích thước kiểu là quyết định thiết kế, nên với thanh ghi, trường gói tin, bộ đếm: ghi kiểu tường minh (`std::uint8_t`, `std::uint32_t`).

### 2. Lambda và ba cách truyền callback

{{code:chapter-16/lambda_callbacks.cpp}}

{{out:chapter-16/lambda_callbacks}}

Một lambda là **một kiểu struct vô danh** do trình biên dịch sinh, có `operator()`. Các biến capture là **thành viên** của struct đó.

- **Lambda không capture** (`[](void* ctx, uint8_t b){...}`) chuyển ngầm được sang con trỏ hàm thường. Đây là cầu nối sang API kiểu C (HAL của nhà sản xuất, FreeRTOS, `signal`). Trạng thái đi qua tham số `ctx`.
- **Lambda có capture + template** (`for_each_byte(..., F&& f)`): `F` là kiểu lambda cụ thể, hàm được sinh riêng cho nó, gọi trực tiếp và thường được inline. Không heap, không con trỏ hàm. Cái giá: mỗi lambda khác nhau sinh một bản `for_each_byte` riêng (Flash tăng nhẹ nếu lạm dụng), và không lưu được vào biến thành viên nếu không biết kiểu.
- **`std::function`**: lưu được mọi callable trong một kiểu duy nhất, nhưng có chi phí gọi gián tiếp và có thể cấp phát heap khi callable lớn hơn vùng lưu trữ nội bộ (kích thước vùng này do thư viện quy định — hãy đo trên toolchain của bạn). Trong firmware không cho phép heap, tránh nó; nếu cần kiểu "function object lưu được, kích thước cố định", dùng thư viện như ETL (`etl::delegate`) — xem chương về chất lượng.

Bảng chọn nhanh:

| Nhu cầu | Chọn |
|---------|------|
| Gọi vào API C, callback đăng ký lúc chạy | Lambda không capture → con trỏ hàm + `void* ctx` |
| Thuật toán/vòng lặp nhận hành vi tùy biến, biết kiểu lúc biên dịch | Template `F&&` + lambda |
| Lưu callable bất kỳ vào member, chấp nhận heap | `std::function` (hiếm khi hợp với nhúng) |

### 3. Capture: nên biết ba quy tắc
1. `[&]`/`[=]` capture-all dễ gây lỗi; ưu tiên liệt kê tường minh (`[&sum]`, `[limit]`).
2. Capture theo tham chiếu một biến cục bộ rồi để lambda **sống lâu hơn** biến đó (ví dụ đăng ký làm callback ngắt) là **dangling reference** — lỗi kinh điển, chỉ lộ ra khi ISR chạy.
3. `sizeof` lambda không capture nhỏ (thường 1 byte); mỗi capture cộng thêm kích thước của nó — in ra bằng `sizeof` như ví dụ để tự kiểm chứng.

### 4. Structured binding
`const auto [lo, hi] = lim;` tách struct/`std::pair`/`std::array` thành các biến có tên. Chỉ là cú pháp; không sao chép thêm nếu bạn dùng `const auto&`.

## Góc nhúng
- Dùng Compiler Explorer với `-O2 --target arm` để so sánh assembly của `for_each_byte` với vòng `for` viết tay: thường giống hệt.
- Nếu callback được gọi từ ISR, không ném exception, không cấp phát heap, và giữ ngắn — lambda cũng không có ngoại lệ nào.
- Khi cần thêm dữ liệu cho callback kiểu C, `void* ctx` trỏ tới một đối tượng **có thời gian sống đủ dài** (static hoặc thành viên của driver), không phải biến trên stack của hàm đăng ký.

## Lỗi thường gặp
- Dùng `auto` cho biến thanh ghi/bộ đếm rồi bị kiểu `int` làm sai phép tràn số.
- Đưa lambda có capture vào tham số kiểu con trỏ hàm: lỗi biên dịch (chỉ lambda không capture mới chuyển được).
- Capture `[&]` biến cục bộ vào callback sống lâu → dangling reference.
- Dùng `std::function` trong đường ISR hoặc vòng nóng mà không đo chi phí.

## Bài tập
1. Sửa `on_rx` để lưu thêm một callback thứ hai `on_error`. Dùng lại cùng mẫu `void*` ctx.
2. Viết `template <typename F> void repeat(int n, F&& f)` gọi `f(i)` n lần; dùng nó để điền một `std::array<uint8_t, 8>` bằng lambda.
3. Đo `sizeof` của một lambda capture hai biến `uint32_t` theo giá trị; giải thích kết quả.
4. Nêu một tình huống trong dự án của bạn nơi `std::function` là chấp nhận được, và một tình huống không.

## Tóm tắt
- `auto` và lambda không tốn chi phí chạy riêng; chi phí nằm ở **cách bạn lưu và truyền** callable.
- Lambda không capture ↔ con trỏ hàm; lambda có capture ↔ template; `std::function` ↔ tiện nhưng có thể dùng heap.
- Đặt tên kiểu tường minh cho dữ liệu phần cứng; để `auto` cho kiểu phức tạp hoặc hiển nhiên.
