---
chapter: 2
title: "Từ ngôn ngữ máy đến C++"
part: 0
code: code/chapter-01
---

# Chương 2. Từ ngôn ngữ máy đến C++

## Mục tiêu
- Nắm mạch lịch sử: ngôn ngữ máy → Assembly → ngôn ngữ cấp cao → C → C++.
- Hiểu vì sao C được thiết kế "gần phần cứng" và C++ kế thừa điều đó.
- Biết các phiên bản chuẩn C++ và nên chọn phiên bản nào cho nhúng.

## Câu chuyện

### Ngôn ngữ máy và Assembly
CPU chỉ hiểu **mã máy**: chuỗi bit mã hoá lệnh. Viết trực tiếp bằng số nhị phân dễ sai và không đọc được, nên xuất hiện **Assembly**: mỗi lệnh máy có một tên gợi nhớ (mnemonic), ví dụ `MOV`, `ADD`, `B` (nhánh). Một *assembler* dịch Assembly sang mã máy theo tỉ lệ gần như một-một.

Hệ quả cho nhúng đến tận ngày nay: mỗi họ CPU (ARM, AVR, RISC-V, x86) có tập lệnh riêng, nên Assembly **không chuyển được** giữa các chip.

### Ngôn ngữ cấp cao đầu tiên
Từ cuối thập niên 1950, các ngôn ngữ như FORTRAN cho phép viết công thức gần toán học; trình biên dịch lo phần dịch sang mã máy. Lập trình viên đổi một phần kiểm soát lấy năng suất và tính di động.

### C: ngôn ngữ hệ thống
Đầu thập niên 1970, tại Bell Labs, Dennis Ritchie tạo ra C để viết lại hệ điều hành Unix — một hệ điều hành trước đó viết bằng Assembly. Yêu cầu thiết kế rất thực dụng: đủ cao để di động giữa các máy, đủ thấp để truy cập bộ nhớ và thanh ghi trực tiếp. Sách *The C Programming Language* (Kernighan & Ritchie, 1978) phổ biến ngôn ngữ; sau đó ANSI/ISO chuẩn hoá thành C89/C90.

Hai đặc điểm của C là nền cho nhúng:
- **Con trỏ** và bộ nhớ phẳng: trỏ thẳng vào địa chỉ thanh ghi.
- **Mô hình chạy tối giản**: không có runtime nặng; `main` chạy sau khi startup code dựng stack.

### C++: C cộng khả năng trừu tượng
Cuối thập niên 1970, Bjarne Stroustrup (Bell Labs) bắt đầu "C with Classes", được đổi tên C++ đầu thập niên 1980. Mục tiêu: giữ hiệu năng và khả năng truy cập phần cứng của C, thêm cách tổ chức chương trình lớn (class, sau này template).

### Các phiên bản chuẩn C++

| Chuẩn | Thêm gì đáng chú ý (với nhúng) |
|-------|-------------------------------|
| C++98/03 | Chuẩn ISO đầu tiên; template, STL |
| C++11 | `auto`, lambda, `constexpr`, `nullptr`, smart pointer, move semantics |
| C++14 | `constexpr` mạnh hơn, generic lambda |
| C++17 | `if constexpr`, `std::optional`, `std::string_view`, structured bindings |
| C++20 | Concepts, `std::span`, `consteval`, `constinit`, designated initializers |
| C++23 | `std::expected`, `if consteval`, các cải tiến `constexpr` |

**Khuyến nghị của sách:** dùng **C++20** khi toolchain hỗ trợ (GCC 10+ cho phần lớn tính năng). Nếu chip bị kẹt ở toolchain cũ, C++17 vẫn đủ cho 90% nội dung. Luôn kiểm tra bảng hỗ trợ tính năng của đúng phiên bản trình biên dịch bạn dùng.

## Kiến thức: "C++ nhúng" nằm ở đâu

```text
              ┌──────────────────────────────┐
              │ C++ đầy đủ (ngoại lệ, RTTI,  │
              │ iostream, STL cấp phát động) │
              ├──────────────────────────────┤
              │ C++ nhúng: RAII, template,   │
              │ constexpr, enum class, span  │
              ├──────────────────────────────┤
              │ Phần chung với C:            │
              │ con trỏ, struct, volatile    │
              └──────────────────────────────┘
```

Sách đi từ đáy lên: bạn luôn có thể quay về phần chung với C khi cần kiểm soát tuyệt đối. Trên thực tế các firmware C++ đều gọi lại thư viện C của nhà sản xuất chip (HAL/CMSIS), nên biết cả hai là bắt buộc.

## Ví dụ
Chương trình đầu tiên bạn sẽ chạy trên PC:

{{code:chapter-01/hello.cpp}}

```bash
g++ -std=c++20 -Wall -Wextra -o hello hello.cpp && ./hello
```

{{out:chapter-01/hello}}

## Góc nhúng
- Startup code (thường do nhà sản xuất cung cấp) đặt con trỏ stack, chép dữ liệu khởi tạo từ Flash sang RAM, xoá vùng `.bss`, gọi **constructor của đối tượng toàn cục** rồi mới gọi `main`. Với C++ bước cuối là quan trọng: đối tượng toàn cục có constructor chạy *trước* `main`.
- Thứ tự khởi tạo giữa các đối tượng toàn cục ở **các file khác nhau** không xác định ("static initialization order fiasco"). Phần 5 quay lại chủ đề này.

## Lỗi thường gặp
- Nhầm mốc lịch sử: hãy ghi lại nguồn khi trích năm cụ thể; các mốc trong chương là mức độ thập niên.
- Chọn chuẩn C++ quá mới mà toolchain của chip chưa hỗ trợ — build lỗi ở phần thư viện chuẩn, không phải ở code của bạn.

## Bài tập
1. Chạy `g++ --version` và cho biết chuẩn C++ nào là mặc định của trình biên dịch bạn (gợi ý: in `__cplusplus`, xem chương 5).
2. Tìm bảng "C++ compiler support" của GCC và cho biết phiên bản GCC tối thiểu hỗ trợ `std::span`.
3. Vì sao Assembly viết cho ARM Cortex-M không chạy được trên AVR?

## Tóm tắt
- Mã máy → Assembly → ngôn ngữ cấp cao → C → C++: mỗi bước đổi một chút kiểm soát lấy năng suất.
- C thiết kế để viết hệ điều hành; C++ giữ nền đó và thêm trừu tượng.
- C++20 là mục tiêu của sách; nhúng dùng một tập con có chủ đích.
