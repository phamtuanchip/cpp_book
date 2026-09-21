---
chapter: 1
title: "Vì sao C++ cho hệ thống nhúng"
part: 0
code: code/chapter-01
---

# Chương 1. Vì sao C++ cho hệ thống nhúng

## Mục tiêu
Sau chương này bạn có thể:
- Chỉ ra **sáu chỗ cụ thể** trong firmware C mà C++ giải quyết tốt hơn, và viết được cả hai phiên bản để so sánh.
- Phân biệt tính năng C++ **miễn phí lúc chạy** với tính năng **có phí**, và biết vì sao "C++ nặng" là nhận định thiếu chính xác.
- Trả lời các phản đối thường gặp trong nhóm: "C nhanh hơn", "C++ phình code", "chuẩn an toàn bắt dùng C".
- Có kế hoạch chuyển dự án C sang C++ **từng bước**, không phải viết lại từ đầu.
- Biết chính xác mình sẽ học phần nào của C++ trong sách này.

**Yêu cầu trước:** đọc được C ở mức cơ bản (biến, hàm, con trỏ, struct). Chưa cần biết C++.

---

## 1. Câu chuyện: firmware lớn lên

Hãy theo dõi một sản phẩm giả định: **bộ đo nhiệt độ không dây** cho kho lạnh. Đây là câu chuyện thường gặp; các con số dòng code chỉ mang tính minh hoạ.

| Thời điểm | Tính năng | Quy mô | Người viết |
|-----------|-----------|--------|-----------|
| Tháng 1 | Đọc một cảm biến, bật LED khi quá ngưỡng | ~300 dòng | 1 người |
| Tháng 6 | Thêm UART log, cảm biến thứ hai (I²C), lưu cấu hình vào Flash | ~5 000 dòng | 2 người |
| Tháng 12 | Thêm radio, máy trạng thái giao thức, cập nhật firmware từ xa, chế độ ngủ tiết kiệm điện | ~30 000 dòng | 3 người |
| Năm 2 | Ba biến thể phần cứng (đổi cảm biến, đổi MCU), nhiều khách hàng, mỗi người một cấu hình | 50 000+ dòng | 4 người, có người mới vào |

Ở tháng 1, C là lựa chọn hoàn hảo. Ở năm 2, các lỗi không còn đến từ cú pháp mà từ **độ phức tạp**:

- Có người truyền sai độ dài bộ đệm; lỗi chỉ lộ ra khi gói tin dài bất thường.
- Có người bật bit sai chân vì macro tên giống nhau ở hai board.
- Có một đường lỗi hiếm quên nhả khoá; thiết bị "treo" sau vài tuần chạy.
- Thêm một cảm biến mới phải sửa ở bảy nơi vì logic chọn thiết bị nằm rải rác trong `#if`.

Đây không phải lỗi của lập trình viên yếu. C chỉ có ít công cụ để **nhờ trình biên dịch canh giữ** những ràng buộc đó. C++ có nhiều hơn — và phần lớn không tốn thêm byte nào.

Phần tiếp theo lần lượt đi qua sáu nỗi đau, mỗi nỗi có code C, code C++, và câu trả lời cho "trả giá gì?".

---

## 2. Sáu chỗ C++ giúp firmware bớt đau

### 2.1 Thanh ghi và chân GPIO: từ macro đến kiểu

**Trong C**, cách phổ biến là macro:

```c
#define GPIOA_ODR   (*(volatile uint32_t*)0x48000014)
#define LED_PIN     5
#define LED_ON()    (GPIOA_ODR |=  (1u << LED_PIN))
#define LED_OFF()   (GPIOA_ODR &= ~(1u << LED_PIN))
```

Vấn đề: macro không có kiểu, không có phạm vi (scope), lỗi hiện ra như lỗi văn bản chứ không phải lỗi logic. `LED_PIN` viết `50` thay vì `5` vẫn biên dịch — và ghi ra rác (hành vi không xác định khi dịch bit quá độ rộng).

**Trong C++**, ta biểu diễn chân bằng template và để `static_assert` canh giới hạn:

{{code:chapter-01/led_c_vs_cpp.cpp}}

Chạy thử:

```bash
g++ -std=c++20 -Wall -Wextra -o led led_c_vs_cpp.cpp && ./led
```

{{out:chapter-01/led_c_vs_cpp}}

Bỏ dấu comment ở dòng `Gpio<40>::set()`: lỗi xuất hiện **lúc biên dịch** ("Pin phải nằm trong 0..31"), không phải lúc đo trên board.

**Giá phải trả?** `Gpio<5>::set()` là một hàm `static` nhỏ; với `-O1` trở lên nó được inline thành cùng lệnh đọc-sửa-ghi như macro. Bạn kiểm chứng bằng Compiler Explorer (mục "Góc nhúng").

### 2.2 Nhiều thiết bị, một giao diện

Firmware có nhiều "cảm biến": nhiệt độ, độ ẩm, áp suất, mỗi loại vài chip. Bạn muốn mã ứng dụng chỉ gọi `read()`.

**Trong C**: struct chứa con trỏ hàm. **Trong C++**: có hai hướng — lớp ảo (đa hình lúc chạy) hoặc template (đa hình lúc biên dịch).

{{code:chapter-01/device_interface.cpp}}

{{out:chapter-01/device_interface}}

So sánh ba cách:

| Tiêu chí | Con trỏ hàm (C) | Lớp ảo (C++) | Template (C++) |
|----------|-----------------|--------------|----------------|
| Ai kiểm tra kiểu | Bạn (ép `void*`) | Trình biên dịch | Trình biên dịch |
| Quên cài hàm `read` | Con trỏ NULL → treo lúc chạy | Lỗi biên dịch (hàm thuần ảo) | Lỗi biên dịch |
| RAM mỗi đối tượng | 1 con trỏ hàm + 1 con trỏ ngữ cảnh | 1 vptr | 0 |
| Gọi hàm | Gián tiếp | Gián tiếp | Trực tiếp, inline được |
| Chọn lúc nào | Chạy | Chạy | Biên dịch |

**Quy tắc chọn:** nếu kiểu thiết bị **biết trước lúc biên dịch** (đa số firmware: board cố định), template là lựa chọn rẻ nhất. Nếu cần hoán đổi lúc chạy (ví dụ cắm nhiều loại cảm biến qua đường tự nhận dạng), dùng lớp ảo. Phần 3 (OOP cho nhúng) bàn kỹ hơn.

### 2.3 Nhả tài nguyên: từ `goto cleanup` đến RAII

Mọi hàm giữ tài nguyên (khoá, DMA channel, chip select, bộ nhớ) đều phải nhả ở **mọi** đường thoát.

{{code:chapter-01/raii_vs_goto.cpp}}

{{out:chapter-01/raii_vs_goto}}

**Cách C** phải nhớ nhảy về nhãn `out` ở mỗi nhánh; khi ai đó thêm một `return` "cho gọn" ở giữa, khoá bị bỏ quên. **Cách C++** dùng nguyên tắc **RAII** (Resource Acquisition Is Initialization): tài nguyên gắn với đối tượng cục bộ; khi ra khỏi phạm vi, destructor **luôn** chạy.

**Giá phải trả?** Destructor của `LockGuard` được inline; mã máy thường giống hệt hai lời gọi `lock()` và `unlock()` tay. Đây là ví dụ kinh điển của "trừu tượng miễn phí".

### 2.4 Hằng số và trạng thái: `enum` và `enum class`

Máy trạng thái giao thức là nơi lỗi "nhầm giá trị" phổ biến nhất.

{{code:chapter-01/enum_class.cpp}}

{{out:chapter-01/enum_class}}

`enum` của C chuyển ngầm thành `int` và các tên nằm ở phạm vi bao ngoài (`Red`, `Green` chiếm chỗ trong namespace hiện tại). `enum class` giữ tên trong phạm vi riêng (`Level::High`), **không** chuyển ngầm, và cho phép chọn kiểu nền (`unsigned char`) — quan trọng khi enum nằm trong gói tin hoặc thanh ghi có độ rộng cố định.

**Giá phải trả?** Không. `enum class` chỉ là kiểu lúc biên dịch.

### 2.5 Cấu hình: từ `#define` và `#if` đến `constexpr`

Cấu hình cố định (xung nhịp, baud rate, kích thước bộ đệm, bảng tra) thường là macro và mảng gõ tay.

{{code:chapter-01/config_constexpr.cpp}}

{{out:chapter-01/config_constexpr}}

Ba điểm đáng nhìn:
1. `uart_divisor` là hàm thật (có kiểu, gỡ lỗi được) nhưng kết quả có sẵn lúc biên dịch; `static_assert` xác nhận luôn.
2. `max_value<Bits>` từ chối `Bits` ngoài 1..32 ngay lúc biên dịch.
3. `kSquares` được tính bằng vòng lặp **trong trình biên dịch**, rồi nằm trong Flash. Bảng sin, CRC, gamma tương tự — bạn thay việc gõ tay hoặc sinh code bằng script ngoài.

**Giá phải trả?** Thời gian biên dịch tăng nhẹ; lúc chạy giảm (không còn tính toán). Đây là chiều ngược lại với nhận định "C++ tốn".

### 2.6 Mảng và độ dài: từ `(p, n)` đến `std::array`/`std::span`

Chữ ký `void send(const uint8_t* p, size_t n)` không ép `n` khớp với dữ liệu. Trong C++ có hai cách gắn độ dài vào kiểu: `std::array<T, N>` (độ dài cố định, biết lúc biên dịch) và `std::span<T>` (một "cửa sổ" nhìn vào dữ liệu, gồm con trỏ + độ dài). Ví dụ bọc một API kiểu C của hãng chip:

{{code:chapter-01/extern_c_demo.cpp}}

{{out:chapter-01/extern_c_demo}}

Đoạn này cũng minh hoạ **`extern "C"`**: cơ chế để C++ gọi mã C (và ngược lại). Nó là cầu nối quan trọng khi chuyển dần dự án — xem mục 5.

**Giá phải trả?** `std::span` là hai từ máy (con trỏ + độ dài) truyền theo giá trị, thường nằm trong thanh ghi. `std::array` không thêm byte nào so với mảng thô.

---

## 3. Nguyên tắc zero-overhead — nói cho đúng

Stroustrup, người thiết kế C++, phát biểu nguyên tắc này thành hai vế:

1. **Cái bạn không dùng, bạn không trả tiền.**
2. **Cái bạn dùng, bạn không thể tự viết tay tốt hơn.**

Với nhúng, đọc thành ba tầng chi phí:

| Tầng | Ví dụ | Bạn trả gì |
|------|-------|-----------|
| **Miễn phí lúc chạy** | `class`, `namespace`, `template`, `constexpr`, `static_assert`, `enum class`, RAII, hàm `inline` | Chỉ tốn thời gian biên dịch |
| **Có phí, bạn chọn** | Hàm ảo, `std::function`, `new`/`delete` | vptr, gọi gián tiếp, heap |
| **Có phí lớn, mặc định bật nhưng tắt được** | Ngoại lệ, RTTI | Kích thước Flash |

Điều đáng nhớ: **phí không nằm ở "C++"** mà ở một số tính năng cụ thể, và ta có công tắc cho từng cái (`-fno-exceptions`, `-fno-rtti`). Chương 3 lập bản đồ chi phí đầy đủ và dạy cách đo.

> Ghi chú về nguồn: ISO/IEC TR 18015 ("Technical Report on C++ Performance") là tài liệu chuẩn phân tích chi phí của từng tính năng; đáng đọc nếu bạn cần lập luận với đồng nghiệp.

---

## 4. Những phản đối thường gặp — và câu trả lời trung thực

| Phản đối | Đúng một phần | Cần làm rõ |
|----------|---------------|-----------|
| "C++ chậm hơn C" | Nếu dùng hàm ảo trong vòng lặp nóng hoặc `new` trong ngắt | Với cùng thuật toán và tính năng tương đương, mã máy thường giống nhau. Hãy **đo** bằng Compiler Explorer |
| "C++ làm phình Flash" | Ngoại lệ, RTTI, `iostream` và instantiate template bừa bãi thật sự tốn | Tắt hoặc tránh chúng; template dùng có kỷ luật thường giảm code nhờ inline và loại bỏ nhánh chết |
| "Không dự đoán được thời gian chạy" | `new`/`delete`, ngoại lệ và container động không xác định thời gian | Chỉ dùng cấp phát tĩnh/lúc khởi tạo; RAII và template có thời gian xác định |
| "Chuẩn an toàn (MISRA, ISO 26262...) bắt dùng C" | Nhiều sản phẩm cũ dùng MISRA C | Có **MISRA C++:2023** và AUTOSAR C++14 cho automotive; công cụ phân tích tĩnh hỗ trợ C++. Kiểm tra yêu cầu chứng nhận cụ thể của bạn |
| "Toolchain chip của tôi chỉ có C" | Một số MCU rất nhỏ hoặc SDK cũ | Đa số toolchain GCC/Clang cho ARM, AVR, RISC-V, ESP32, Xtensa hỗ trợ C++; một số SDK (Arduino, Mbed OS, ESP-IDF) cung cấp API C++ hoặc cho phép dùng C++ |
| "Cả đội quen C" | Chi phí đào tạo có thật | Chuyển từng bước (mục 5); nhiều lợi ích đến từ 5–6 tính năng đơn giản, không cần học hết ngôn ngữ |

Sách không nói "C++ luôn tốt hơn". Có những dự án nên giữ C: MCU vài KB Flash, đội hoàn toàn quen C và không có nhu cầu mở rộng, yêu cầu chứng nhận với toolchain chỉ được duyệt cho C. **Quyết định dựa trên đo đạc và ràng buộc của bạn**, không phải thói quen hay tín ngưỡng.

---

## 5. Chuyển từ C sang C++ từng bước

Không cần viết lại. Một lộ trình an toàn:

| Bước | Việc làm | Rủi ro |
|------|----------|--------|
| 0 | Đo hiện trạng: `size`, map file, số cảnh báo | Không |
| 1 | Biên dịch **file C hiện có bằng C++** (đổi phần mở rộng hoặc cờ). Sửa các khác biệt (ép `void*`, `goto` qua khởi tạo, tên biến trùng từ khoá C++ như `class`, `new`) | Thấp |
| 2 | Thêm `extern "C"` quanh header C để gọi từ C++ | Thấp |
| 3 | Viết **mã mới** bằng C++ (driver mới, module mới); chỉ dùng phần miễn phí (bảng mục 3) | Thấp |
| 4 | Dần thay macro bằng `constexpr`/`enum class`/`inline`; thay `goto cleanup` bằng RAII | Trung bình |
| 5 | Đo lại; so với bước 0 | — |

Mẫu header C dùng được từ cả hai ngôn ngữ:

```c
/* uart.h */
#ifndef UART_H
#define UART_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_write(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* UART_H */
```

`extern "C"` báo trình biên dịch C++ **không** làm "biến đổi tên" (name mangling) cho các hàm này, để trình liên kết khớp được với tệp `.c` đã biên dịch bằng trình biên dịch C.

Nguyên tắc: **không đổi hai thứ cùng lúc**. Đổi compiler và đổi kiến trúc ở hai lần commit riêng, đo sau mỗi lần.

---

## 6. Sách này dạy phần nào của C++

| Dùng thoải mái | Dùng có cân nhắc | Thường tránh trên MCU nhỏ |
|----------------|------------------|---------------------------|
| `constexpr`, `template`, `static_assert` | Hàm ảo, giao diện trừu tượng | Ngoại lệ (`throw`) |
| RAII, `enum class`, `std::array` | Kế thừa nhiều tầng | RTTI (`dynamic_cast`, `typeid`) |
| `auto`, lambda không capture | Cấp phát động lúc khởi tạo | `new`/`delete` trong vòng lặp chạy |
| `std::span`, `std::optional` | `std::function` | Container cấp phát động không có allocator riêng |
| `namespace`, hàm `inline` | `std::string` | `iostream` |

Đọc bảng này từ trái sang phải như **bậc thang tin cậy**. Sách đi đúng thứ tự đó: Phần 2–4 tập trung cột trái; Phần 5–6 đưa vào cột giữa khi có lý do; cột phải được thảo luận khi cần để bạn biết vì sao nên tránh.

---

## 7. Góc nhúng: tự kiểm chứng "không tốn phí"

Bạn không nên tin chương này chỉ vì nó nói vậy. Cách kiểm chứng trong 5 phút:

1. Mở https://godbolt.org, chọn ngôn ngữ **C++**.
2. Chọn trình biên dịch ARM: `ARM GCC` (hoặc `AVR gcc`, `RISC-V` theo chip của bạn).
3. Ô cờ biên dịch: `-std=c++20 -O2`.
4. Dán đoạn sau (không cần `main`):

```cpp
#include <cstdint>

extern volatile std::uint32_t g_odr;

template <unsigned Pin>
struct Gpio {
    static void set() { g_odr |= (1u << Pin); }
};

void via_template() { Gpio<5>::set(); }
void via_macro()    { g_odr |= (1u << 5); }
```

5. So sánh phần assembly của `via_template` và `via_macro`. Bạn sẽ thấy hai hàm cùng dạng: nạp địa chỉ, đọc, đặt bit, ghi.
6. Đổi `-O2` thành `-O0`: mã của template dài hơn. Đây là lý do **luôn đo với cờ tối ưu giống bản phát hành** (`-Os`/`-O2`).

Với mã thật trên máy bạn, dùng:

```bash
arm-none-eabi-size build/firmware.elf
arm-none-eabi-nm --size-sort -S build/firmware.elf | tail -20
```

---

## 8. Lỗi thường gặp

| Sai lầm | Hệ quả | Cách tránh |
|---------|--------|-----------|
| Coi C++ là "C có class" và dùng mọi tính năng | Firmware phình, thời gian mất dự đoán | Bám bảng ở mục 6 |
| Đo kích thước với `-O0` | Kết luận sai "C++ nặng" | Đo với `-Os`/`-O2` |
| Thêm `#include <iostream>` để in log | Kéo cả thư viện luồng vào Flash | Dùng `printf` nhỏ gọn hoặc hàm UART riêng |
| Bỏ `volatile` với thanh ghi thật | Trình biên dịch xoá lệnh "thừa" | Thanh ghi và biến chia sẻ với ngắt luôn `volatile` (Phần 5) |
| Bật ngoại lệ mặc định trên MCU nhỏ | Flash tăng, hành vi khi `throw` khó chứng minh | `-fno-exceptions` từ đầu dự án |
| Đổi compiler và kiến trúc cùng lúc | Không biết lỗi từ đâu | Từng bước, mỗi bước một commit |
| Lấy con số "C++ tốn X%" từ bài viết | Không áp dụng được cho build của bạn | Tự đo trong dự án của mình |

---

## 9. Bài tập

**Cơ bản**
1. Thêm hàm `toggle()` vào `Gpio<Pin>` (đảo bit bằng XOR) và kiểm tra bằng `main` chạy trên PC.
2. Bỏ dấu comment `Gpio<40>::set()` trong `led_c_vs_cpp.cpp`. Đọc thông báo lỗi và tìm dòng chứa chuỗi "Pin phải nằm trong 0..31".
3. Trong `enum_class.cpp`, bỏ comment dòng `int b = Level::High;`. Chép lại nguyên văn thông báo lỗi.

**Trung bình**
4. Trong `raii_vs_goto.cpp`, thêm nhánh `fail_at == 3` vào cả hai hàm nhưng **quên** `goto out` ở bản C. Chạy và giải thích giá trị `g_locked` in ra.
5. Viết `average_of_two` cho một loại cảm biến thứ hai (ví dụ `FakeHumidity`) mà **không** sửa hàm template. Nêu nhận xét về việc này so với thêm một phần tử vào `struct` con trỏ hàm.
6. Viết `constexpr` hàm `crc8_table()` sinh bảng 256 phần tử CRC-8 (đa thức 0x07) và `static_assert` rằng `table[1] == 0x07`.

**Nâng cao**
7. Chọn một module C có thật trong dự án của bạn (khoảng 100–300 dòng). Thực hiện bước 1 ở mục 5 (biên dịch bằng C++). Ghi lại mọi lỗi biên dịch và nhóm chúng theo nguyên nhân.
8. Dùng Compiler Explorer với chip đích của bạn để so sánh mã máy giữa `virtual` và `template` cho `read()`. Ghi số lệnh và cách gọi (gián tiếp hay trực tiếp).

---

## 10. Tóm tắt

- Firmware lớn lên thì vấn đề chuyển từ cú pháp sang **quản lý độ phức tạp**; C++ cho công cụ để trình biên dịch canh giữ ràng buộc.
- Sáu chỗ được cải thiện: thanh ghi/GPIO có kiểu, giao diện thiết bị, RAII, `enum class`, `constexpr`, `std::array`/`std::span`.
- Phần lớn tính năng C++ **không tốn phí lúc chạy**; phí thật nằm ở ngoại lệ, RTTI, heap, hàm ảo, `iostream` — và ta có công tắc cho từng cái.
- Chuyển từ C nên đi từng bước: biên dịch bằng C++, `extern "C"`, viết mã mới bằng C++, dần thay macro.
- Không có câu trả lời chung cho mọi dự án: **đo trước, kết luận sau**.

## Đọc thêm
- Bjarne Stroustrup, *The Design and Evolution of C++* — lý do thiết kế và nguyên tắc zero-overhead.
- ISO/IEC TR 18015:2006, *Technical Report on C++ Performance* — phân tích chi phí từng tính năng.
- *C++ Core Guidelines* (isocpp.github.io/CppCoreGuidelines) — hướng dẫn thực hành; phần về RAII, `enum class`, `constexpr`.
- Tài liệu toolchain của chip bạn dùng: xem mục hỗ trợ C++, newlib/newlib-nano, `-fno-exceptions`.
