---
chapter: 15
title: "Template cơ bản và đa hình tĩnh (CRTP)"
part: 3
code: code/chapter-15
---

# Chương 15. Template cơ bản và đa hình tĩnh (CRTP)

## Mục tiêu
- Viết template hàm với ràng buộc kiểu bằng `concept` (C++20).
- Hiểu CRTP (Curiously Recurring Template Pattern): cách đạt "đa hình" mà không cần vtable.
- Biết khi nào chọn template thay cho lớp ảo (và ngược lại) dựa trên tiêu chí cụ thể, không phải sở thích.

## Câu chuyện: driver UART và SPI dùng chung khung, không muốn trả phí vtable
Một thư viện driver có `UartDriver` và `SpiDriver`, cả hai đều có `init()` và `write()`. Dùng lớp cơ sở ảo (chương 14) là cách hợp lý — **nếu** loại driver được chọn lúc chạy. Nhưng trong firmware này, loại driver được **chọn lúc biên dịch** (mỗi bản build chỉ nhắm một board cụ thể): không có lý do gì để trả phí vtable và gọi gián tiếp cho một quyết định đã biết trước. CRTP giải quyết đúng bài toán này.

## Kiến thức

### 1. Template hàm và `concept`

{{code:chapter-15/function_templates.cpp}}

{{out:chapter-15/function_templates}}

- `template <Arithmetic T> constexpr T clamp(...)`: `Arithmetic` là một **concept** — ràng buộc kiểu `T` phải thoả `std::is_arithmetic_v<T>`. Nếu ai đó gọi `clamp` với một `struct` không phải số, lỗi hiện ra **ngay tại điểm gọi** với thông báo dễ hiểu, thay vì lỗi khó đọc từ sâu bên trong thân hàm (đây là vấn đề kinh điển của template trước C++20).
- `clamp_static<Lo, Hi>(value)` dùng **tham số template không phải kiểu** (`int Lo, int Hi`): giới hạn trở thành hằng số lúc biên dịch, không tốn RAM lưu `lo`/`hi`, và `static_assert(Lo <= Hi, ...)` kiểm tra ngay khi khai báo instantiation — không đợi tới lúc chạy mới phát hiện cấu hình sai.
- Cả `clamp` lẫn `clamp_static` đều `constexpr`: dùng được trong `static_assert`.

### 2. CRTP: đa hình lúc biên dịch

{{code:chapter-15/crtp_driver.cpp}}

{{out:chapter-15/crtp_driver}}

Cách đọc `DriverBase`:

```cpp
template <typename Derived>
class DriverBase {
public:
    void init() { static_cast<Derived*>(this)->init_impl(); }
};
class UartDriver : public DriverBase<UartDriver> { ... };
```

`UartDriver` kế thừa từ `DriverBase<UartDriver>` — **truyền chính mình** làm tham số template cho lớp cơ sở. Bên trong `DriverBase::init()`, `static_cast<Derived*>(this)` ép `this` (kiểu `DriverBase<UartDriver>*`) về đúng `UartDriver*`, rồi gọi `init_impl()` của nó. Toàn bộ việc này xảy ra **lúc biên dịch**: với mỗi `Derived` khác nhau, trình biên dịch sinh ra một phiên bản `DriverBase<...>::init()` riêng, gọi thẳng `init_impl()` tương ứng — **không có vtable, không có con trỏ hàm, không có gọi gián tiếp**. Với `-O2`, `send_hello(uart)` thường được biên dịch thành các lệnh gọi trực tiếp, có thể được inline hoàn toàn.

Đổi lại, cái giá của CRTP:
- Không thể có `std::array<DriverBase*, N>` chứa lẫn lộn `UartDriver` và `SpiDriver` — mỗi `DriverBase<X>` là một **kiểu khác nhau** về mặt ngôn ngữ, không có "kiểu cơ sở chung" để gom lại lúc chạy.
- Thông báo lỗi khi dùng sai template thường dài và khó đọc hơn lỗi với lớp ảo (dù `concept` giúp cải thiện đáng kể, như ở mục 1).
- Code sinh ra cho mỗi `Derived` là một bản riêng — nếu `init_impl()` lớn và có nhiều loại `Derived`, Flash có thể phình (đánh đổi ngược với chương 3: đây là trường hợp hiếm mà template **có thể** tốn Flash hơn một hàm ảo dùng chung).

### 3. Bảng quyết định: lớp ảo hay template?

| Tiêu chí | Chọn lớp ảo (chương 14) | Chọn template/CRTP |
|----------|--------------------------|---------------------|
| Loại cụ thể biết lúc nào | Lúc chạy (đọc từ cấu hình, chân ID, EEPROM) | Lúc biên dịch (mỗi bản build một board) |
| Cần chứa nhiều loại khác nhau trong một mảng/danh sách | Có (`std::array<IBase*, N>`) | Không trực tiếp (mỗi kiểu instantiate riêng) |
| Tần suất gọi | Vài lần/giây — chi phí gọi gián tiếp không đáng kể | Vòng lặp nóng (hàng nghìn lần/giây) — muốn inline |
| Số lượng "loại" khác nhau trong cùng bản build | Nhiều, hoặc mở rộng bởi bên thứ ba | Ít, cố định, biết trước |
| Độ rõ thông báo lỗi khi dùng sai | Rõ ràng hơn | Kém hơn (dù `concept` cải thiện nhiều) |

Không có lựa chọn "luôn đúng". Nguyên tắc chương 1 vẫn áp dụng: khi không chắc, viết bản đơn giản (thường là lớp ảo, dễ đọc hơn), rồi đo, rồi đổi sang CRTP **nếu** đo thấy cần.

## Ví dụ thực tiễn: HAL mỏng viết bằng CRTP
Nhiều framework nhúng hiện đại (không chỉ C++) dùng đúng mẫu này cho lớp trừu tượng phần cứng (HAL) mỏng nhất có thể: mỗi board có một file cấu hình instantiate `GpioPin<PortA, 5>`, `UartDriver<Usart1>`... — toàn bộ được giải quyết lúc biên dịch, firmware cuối cùng chỉ chứa đúng code cho đúng phần cứng của board đó, không có nhánh rẽ hay bảng hàm ảo thừa.

## Góc nhúng
- Kiểm tra bằng Compiler Explorer: biên dịch `send_hello(uart)` với `-O2` và xem assembly — nó thường ngắn gọn như thể bạn gọi thẳng `uart.init_impl()`.
- Nếu có **nhiều** loại driver và hàm `init_impl()`/`write_impl()` lớn, đo tổng kích thước Flash của bản CRTP so với bản dùng một lớp cơ sở ảo chung — đôi khi lớp ảo lại nhỏ hơn vì code được **dùng chung** thay vì nhân bản.
- CRTP không dùng `virtual` nên không có vtable — hữu ích khi mục tiêu là **loại bỏ hoàn toàn** vtable trong toàn bộ firmware (một số chuẩn an toàn/MISRA hạn chế nghiêm ngặt đa hình động).

## Lỗi thường gặp
- Dùng CRTP "vì nó nhanh hơn" mà chưa đo, trong khi bài toán thực ra cần chọn loại lúc chạy — kết quả là phải viết lại bằng lớp ảo sau này.
- Quên rằng `UartDriver` và `SpiDriver` không có kiểu cơ sở chung nào ở đây — cố gán `DriverBase<UartDriver>*` cho `DriverBase<SpiDriver>*` là lỗi biên dịch (hai kiểu khác nhau), không phải điều gì khắc phục được bằng ép kiểu.
- Viết `concept` quá lỏng (hoặc bỏ qua, dùng `typename` trần) khiến lỗi sai kiểu hiện ra từ sâu trong thân hàm, khó đọc.
- Nhầm tham số template không phải kiểu (`template <int Lo, int Hi>`) với tham số hàm thường — giá trị phải là hằng số biết lúc biên dịch, không dùng được với biến đọc từ cảm biến lúc chạy.

## Bài tập
1. Thêm `class I2cDriver : public DriverBase<I2cDriver>` với `init_impl()`/`write_impl()` riêng; gọi `send_hello(i2c)`.
2. Viết một `concept UnsignedInteger` (ràng buộc kiểu số nguyên không dấu) và một hàm `constexpr` dùng nó để tính giá trị lớn nhất biểu diễn được của kiểu đó (gợi ý: dùng `std::numeric_limits`).
3. Thử viết một mảng chứa cả `UartDriver` và `SpiDriver` để gọi `init()` qua vòng lặp, **không dùng** lớp cơ sở ảo. Giải thích vì sao cách trực tiếp không được, và nêu ít nhất một cách vòng (ví dụ: gọi riêng từng cái, hoặc viết một hàm nhận template pack) — không cần cài đặt đầy đủ.
4. So sánh bằng lời: sự khác nhau giữa `template <typename Derived> class DriverBase` (CRTP, đa hình tĩnh) và `template <Arithmetic T> T clamp(...)` (template thông thường) — cả hai đều là "template", nhưng giải quyết hai loại bài toán khác nhau. Nêu rõ khác biệt đó là gì.

## Tóm tắt
- Template hàm với `concept` cho lỗi rõ ràng hơn khi dùng sai kiểu, so với template không ràng buộc.
- CRTP đạt hiệu ứng tương tự đa hình nhưng giải quyết hoàn toàn lúc biên dịch: không vtable, không gọi gián tiếp, có thể inline.
- Đổi lại, CRTP không cho phép gom nhiều loại khác nhau vào một danh sách lúc chạy, và thông báo lỗi thường khó đọc hơn.
- Chọn giữa lớp ảo và CRTP dựa trên: loại biết lúc nào, tần suất gọi, và nhu cầu chứa nhiều loại trong một tập hợp — không dựa trên cảm tính.
