---
chapter: 5
title: "Môi trường phát triển và chương trình đầu tiên"
part: 1
code: code/chapter-05
---

# Chương 5. Môi trường phát triển và chương trình đầu tiên

## Mục tiêu
- Cài trình biên dịch C++ cho PC (host) và biết vai trò của toolchain chéo (cross) cho MCU.
- Biên dịch bằng dòng lệnh và bằng CMake.
- Kiểm tra được chuẩn C++ và trình biên dịch đang dùng.

## Câu chuyện: hai máy, hai trình biên dịch
Firmware được **biên dịch trên PC** nhưng **chạy trên MCU**. Vì vậy bạn dùng hai loại toolchain:

| Loại | Ví dụ | Dùng để |
|------|-------|---------|
| Host (native) | `g++`, `clang++`, MSVC | Chạy ví dụ và unit test ngay trên PC |
| Cross | `arm-none-eabi-g++`, `avr-g++`, `riscv-none-elf-g++` | Tạo file `.elf` cho chip |

Chiến lược xuyên suốt sách: **viết logic độc lập phần cứng, chạy và test trên host**; chỉ lớp mỏng chạm thanh ghi mới cần chip thật.

## Kiến thức

### Cài trình biên dịch host

| Hệ điều hành | Cách cài |
|--------------|----------|
| Windows | MSYS2 (https://www.msys2.org) rồi trong shell MinGW-w64: `pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-gdb` |
| Ubuntu/Debian | `sudo apt install build-essential cmake gdb` |
| macOS | `xcode-select --install` hoặc `brew install gcc cmake` |

Kiểm tra:

```bash
g++ --version
cmake --version
```

### Toolchain chéo cho MCU
- **ARM Cortex-M:** gói *Arm GNU Toolchain* (`arm-none-eabi-*`), có cho Windows/Linux/macOS trên trang của Arm.
- **AVR:** `avr-gcc` (kèm Arduino IDE hoặc `apt install gcc-avr`).
- **RISC-V:** `riscv-none-elf-gcc` từ xPack hoặc nhà sản xuất chip.
- Nhiều hãng cung cấp IDE đóng gói sẵn (STM32CubeIDE, ESP-IDF, PlatformIO); phần dưới của sách dùng dòng lệnh + CMake để bạn hiểu điều IDE đang làm.

### Cờ biên dịch nên có
| Cờ | Tác dụng |
|----|---------|
| `-std=c++20` | Chọn chuẩn ngôn ngữ |
| `-Wall -Wextra` | Bật cảnh báo hữu ích |
| `-Os` / `-O2` | Tối ưu kích thước / tốc độ |
| `-fno-exceptions -fno-rtti` | Bỏ tính năng đắt (chỉ khi cần) |
| `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections` | Loại code không dùng |

## Ví dụ: kiểm tra môi trường

{{code:chapter-05/main.cpp}}

Cách 1: dòng lệnh.

```bash
cd code/chapter-05
g++ -std=c++20 -Wall -Wextra -o check_env main.cpp && ./check_env
```

Cách 2: CMake — cấu hình dùng lại được cho cả host lẫn chip.

{{code:chapter-05/CMakeLists.txt}}

```bash
cmake -S . -B build
cmake --build build
./build/check_env
```

Kết quả:

{{out:chapter-05/main}}

Nếu `__cplusplus` in ra `202002L` là bạn đang ở C++20. Nếu ra `201703L`, bạn đang ở C++17 và cần kiểm tra cờ `-std=`.

## Góc nhúng
- Với toolchain chéo, CMake cần một **toolchain file** khai báo compiler, cờ CPU (`-mcpu=cortex-m4 -mthumb`), linker script. Phần 6 xây một file như vậy.
- Nên gắn cùng `CMAKE_CXX_STANDARD` cho cả host và target để code test trên PC không khác code chạy trên chip.
- Đừng chỉ tin trình biên dịch trên PC: chạy thêm một lần build bằng toolchain chéo trong CI để bắt lỗi khác biệt (kiểu `int` 16-bit trên AVR, ví dụ).

## Lỗi thường gặp
- `g++: command not found` sau khi cài MSYS2: bạn chưa mở đúng shell MinGW-w64 hoặc chưa thêm thư mục `bin` vào `PATH`.
- Lẫn `gcc` và `g++`: `gcc` biên dịch file `.cpp` nhưng không tự liên kết thư viện chuẩn C++ trong mọi trường hợp; dùng `g++`.
- Quên `-std=c++20` rồi dùng `std::span` — lỗi "not a member of std".
- Chạy `cmake --build` rồi sửa CMakeLists nhưng không cấu hình lại: xoá thư mục `build`.

## Bài tập
1. Cài toolchain host, chạy `check_env` và ghi lại phiên bản compiler + `__cplusplus`.
2. Đổi `CMAKE_CXX_STANDARD` sang 17 rồi 23. Giá trị `__cplusplus` thay đổi thế nào?
3. Cài `arm-none-eabi-g++`. Chạy `arm-none-eabi-g++ --version` và so sánh với `g++`.
4. Biên dịch `main.cpp` với `-Wall -Wextra -Wpedantic`. Có cảnh báo mới nào không?

## Tóm tắt
- Hai loại toolchain: host để chạy/test, cross để tạo firmware.
- Luôn ghi rõ `-std=` và bật `-Wall -Wextra`.
- CMake giúp dùng chung cấu hình giữa host và target.
- Xác nhận môi trường bằng `__cplusplus` trước khi viết code thật.
