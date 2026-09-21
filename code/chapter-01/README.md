# Chương 1 — ví dụ
Biên dịch từng file: `g++ -std=c++20 -Wall -Wextra -o demo <file>.cpp && ./demo`

| File | Nội dung |
|------|----------|
| hello.cpp | Chương trình đầu tiên |
| led_c_vs_cpp.cpp | Macro thanh ghi (C) và `Gpio<Pin>` (C++) |
| device_interface.cpp | Con trỏ hàm, lớp ảo, template |
| raii_vs_goto.cpp | `goto cleanup` và RAII |
| enum_class.cpp | `enum` và `enum class` |
| config_constexpr.cpp | Cấu hình, bảng tra lúc biên dịch |
| extern_c_demo.cpp | Bọc API kiểu C bằng lớp C++ |
