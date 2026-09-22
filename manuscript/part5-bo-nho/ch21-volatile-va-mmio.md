---
chapter: 21
title: "volatile và Memory-Mapped I/O"
part: 5
code: code/chapter-21
---

# Chương 21. `volatile` và Memory-Mapped I/O

## Mục tiêu
- Hiểu chính xác `volatile` báo điều gì cho trình biên dịch, và vì sao thiếu nó là lỗi im lặng.
- Mô hình hoá khối thanh ghi ngoại vi (GPIO, UART...) bằng struct `volatile` thay vì địa chỉ rời rạc.
- Phân biệt `volatile` với `std::atomic`: cái nào giải quyết vấn đề gì, và khi nào cần cả hai.

## Câu chuyện: vòng lặp chờ cờ không bao giờ dừng
```cpp
bool ready = false;
while (!ready) { /* cho ngat bao "xong" */ }
```
Trên bàn debug, biến này được ISR đặt `true`. Nhưng ở bản release (`-O2`), chương trình treo mãi. Trình biên dịch thấy `ready` không hề bị gán bên trong vòng lặp (theo góc nhìn của nó — nó không biết ISR chạy "song song"), nên tối ưu hoá thành `if (!ready) while(true);` — chỉ đọc `ready` **một lần**. Thêm `volatile bool ready` báo: "giá trị này có thể đổi bởi thứ nằm ngoài luồng thực thi bình thường — đọc lại thật mỗi lần, đừng cache trong thanh ghi CPU."

## Kiến thức

### 1. `volatile` nói gì và không nói gì
`volatile T` buộc trình biên dịch:
- Đọc thật từ bộ nhớ mỗi lần truy cập (không dùng giá trị đã cache trong thanh ghi CPU hay tối ưu bỏ đọc "trùng lặp").
- Ghi thật mỗi lần gán (không gộp/loại bỏ các lần ghi liên tiếp mà nó cho là "thừa").
- Giữ **thứ tự** các truy cập `volatile` so với nhau (không sắp xếp lại việc đọc/ghi hai biến `volatile` khác nhau).

`volatile` **không** đảm bảo:
- **Tính nguyên tử (atomicity)**: đọc-sửa-ghi một biến `volatile` (`x++`) vẫn có thể bị chen ngang giữa chừng bởi ISR hay lõi khác.
- **Rào chắn bộ nhớ (memory barrier)** đầy đủ giữa CPU đa lõi: trên hệ đa lõi, thứ tự nhìn thấy bởi lõi khác cần thêm cơ chế đồng bộ (atomic với `memory_order`, hoặc lệnh rào chắn của kiến trúc).
- Rằng biến đó là **thread-safe** theo nghĩa chuẩn C++ (`volatile` không phải công cụ đồng bộ hoá đa luồng — dù nó vẫn cần thiết khi biến được đổi bởi ISR).

### 2. Memory-Mapped I/O (MMIO): thanh ghi phần cứng là địa chỉ bộ nhớ

{{code:chapter-21/mmio_register.cpp}}

{{out:chapter-21/mmio_register}}

Trên hầu hết MCU, thanh ghi ngoại vi **không phải RAM thật** — đọc/ghi một địa chỉ cố định (ví dụ `0x48000014`) sẽ được bus định tuyến tới mạch điều khiển GPIO thay vì ô nhớ. Với C++, cách sạch nhất là một **struct có các trường `volatile`**, đặt đúng thứ tự và kiểu khớp với datasheet (nhắc lại chương 11: không có đệm bất ngờ), rồi ép một con trỏ hằng tới địa chỉ cơ sở:

```cpp
GpioRegs* const GPIOA = reinterpret_cast<GpioRegs*>(0x48000000);
```

So với cách viết `#define`/con trỏ rời rạc cho từng thanh ghi (chương 1), struct nhóm các thanh ghi liên quan lại, cho phép truyền `GpioRegs*` như một "handle" tới hàm dùng chung cho nhiều cổng GPIO (`gpio_set_output(GpioRegs*, unsigned pin)` dùng được cho GPIOA, GPIOB, ...) — vẫn không có chi phí runtime nào so với thao tác con trỏ tay.

**Vì sao mỗi trường phải là `volatile`, không phải cả con trỏ?** `volatile std::uint32_t moder` nghĩa là *giá trị tại địa chỉ đó* có thể đổi bất ngờ (đúng bản chất phần cứng); con trỏ `GpioRegs*` bản thân nó không cần `volatile` vì địa chỉ cơ sở không đổi.

### 3. `volatile` so với `std::atomic`

{{code:chapter-21/volatile_vs_atomic.cpp}}

{{out:chapter-21/volatile_vs_atomic}}

| | `volatile` | `std::atomic` |
|---|---|---|
| Mục đích | Nói "đừng tối ưu, đọc/ghi thật" (dùng cho MMIO, cờ đơn giản) | Đảm bảo nguyên tử + thứ tự bộ nhớ giữa các luồng/lõi |
| Đọc-sửa-ghi an toàn? | Không | Có (`fetch_add`, `compare_exchange`...) |
| Chi phí | Không thêm lệnh (chỉ ảnh hưởng tối ưu hoá) | Có thể thêm lệnh rào chắn/khoá tuỳ kiến trúc |
| Dùng cho thanh ghi phần cứng | **Đúng** — bản chất MMIO là "đọc/ghi thật, không cache" | Không cần thiết, đôi khi không hợp lệ với vùng MMIO |
| Dùng cho biến chia sẻ ISR ↔ main, chỉ đọc/ghi đơn giản một hướng | Đủ (một người ghi, một người đọc, kiểu đủ nhỏ để đọc/ghi nguyên tử tự nhiên trên kiến trúc đó) | Cũng đúng, và rõ ràng hơn về ý định |
| Dùng cho bộ đếm được cả ISR lẫn main cùng tăng | **Không đủ** (đọc-sửa-ghi không nguyên tử) | **Cần** |

Quy tắc thực dụng: **thanh ghi phần cứng → `volatile`**; **biến chia sẻ với ISR/luồng khác mà có đọc-sửa-ghi hoặc cần đảm bảo thứ tự → `std::atomic`**. Nhiều dự án dùng cả hai cho cùng một biến khi cần (ví dụ `volatile std::atomic<uint32_t>` cho thanh ghi vừa MMIO vừa có thao tác đọc-sửa-ghi từ phần mềm — hiếm, nhưng có).

## Góc nhúng
- Đọc-sửa-ghi thanh ghi (`gpio->moder |= ...`) không phải phép toán nguyên tử: nếu cả main loop lẫn ISR cùng sửa **cùng một thanh ghi**, cần tắt ngắt tạm thời (critical section) quanh thao tác đó — chương về ngắt (Phần 6) đi sâu hơn.
- Trên Cortex-M, một số thanh ghi hỗ trợ **bit-banding** hoặc thanh ghi `BSRR` (set/reset riêng) để set/clear từng bit bằng **một lệnh ghi nguyên tử phần cứng**, tránh hẳn nhu cầu đọc-sửa-ghi cho thao tác đơn giản — kiểm tra datasheet MCU của bạn.
- `-O2` có thể sắp xếp lại các truy cập không phải `volatile`; luôn kiểm tra assembly (Compiler Explorer/`objdump -d`) khi nghi ngờ trình tự đọc/ghi thanh ghi không đúng như ý.

## Lỗi thường gặp
- Quên `volatile` trên thanh ghi phần cứng hoặc cờ chia sẻ với ISR — lỗi chỉ lộ ra ở bản tối ưu hoá, không lộ ở `-O0` (nên phải test cả build release).
- Dùng `volatile` cho biến được **nhiều luồng** cùng đọc-sửa-ghi, tưởng vậy là "an toàn luồng" — vẫn có race condition.
- Định nghĩa struct MMIO mà không kiểm tra đệm/thứ tự trường khớp datasheet (chương 11).
- Đọc `IDR` (chỉ đọc theo phần cứng) rồi lỡ tay ghi vào nó trong code — biên dịch không báo lỗi trừ khi bạn khai báo `const volatile`.

## Bài tập
1. Thêm thanh ghi `pupdr` (pull-up/pull-down) vào `GpioRegs` đúng vị trí offset của STM32 thật (tra datasheet) và viết hàm cấu hình pull-up cho một chân.
2. Đánh dấu `idr` là `const volatile std::uint32_t` (chỉ đọc) và thử ghi vào nó — đọc thông báo lỗi biên dịch.
3. Viết một cờ `volatile bool` được ISR giả lập đặt và vòng lặp chính xoá; đo bằng Compiler Explorer xem `-O0` và `-O2` có sinh assembly khác nhau cho vòng lặp chờ hay không nếu **bỏ** `volatile`.
4. Giải thích vì sao `std::atomic<uint32_t> counter; counter++;` an toàn khi cả ISR và main cùng gọi, còn `volatile uint32_t counter; counter++;` thì không, dù cả hai đều "nhìn thấy" giá trị mới nhất.

## Tóm tắt
- `volatile` buộc đọc/ghi thật và giữ thứ tự tương đối giữa các truy cập `volatile`; nó không đảm bảo tính nguyên tử hay đồng bộ đa luồng.
- Mô hình hoá thanh ghi ngoại vi bằng struct có trường `volatile`, ép kiểu một lần từ địa chỉ cơ sở — sạch và tái dùng được hơn định nghĩa rời rạc.
- Thanh ghi phần cứng dùng `volatile`; biến chia sẻ cần đọc-sửa-ghi an toàn giữa ISR/luồng dùng `std::atomic`.
