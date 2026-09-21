---
chapter: 6
title: "Biên dịch chéo, bộ nhớ và gỡ lỗi"
part: 1
code: code/chapter-06
---

# Chương 6. Biên dịch chéo, bộ nhớ và gỡ lỗi

## Mục tiêu
- Hiểu đường đi từ file `.cpp` đến firmware nằm trong Flash.
- Đọc được kích thước firmware (`text`, `data`, `bss`) và biết mỗi biến của mình rơi vào đâu.
- Viết được toolchain file CMake cho biên dịch chéo.
- Biết bộ công cụ gỡ lỗi cơ bản: gdb + SWD, log, logic analyzer, và cách đọc một "hard fault".

## Câu chuyện: "nó chạy trên PC mà"
Nhiều kỹ sư mới gặp cảnh: code chạy hoàn hảo trên PC, nạp vào board thì không nhấp nháy, không in gì cả. Nguyên nhân thường không nằm ở logic mà ở **những thứ PC làm hộ bạn và MCU thì không**: hệ điều hành dựng stack và heap, nạp chương trình vào RAM, khởi tạo biến, in ra màn hình. Trên MCU, chuỗi việc đó do **bạn** (hoặc gói khởi động của hãng chip) đảm nhận. Hiểu chuỗi đó là tiền đề để gỡ lỗi mọi thứ về sau.

## Kiến thức

### 1. Từ mã nguồn đến chip

```text
main.cpp ──(compile)──► main.o ─┐
driver.cpp ─(compile)─► driver.o ─┼─(link + linker script)─► firmware.elf ─(objcopy)─► firmware.bin/.hex ─(flash)─► chip
startup.s / startup.cpp ─► ...   ─┘
```

| Bước | Công cụ | Việc làm |
|------|---------|----------|
| Biên dịch | `arm-none-eabi-g++ -c` | Mỗi `.cpp` thành một `.o` (mã máy + ký hiệu chưa có địa chỉ) |
| Liên kết | `arm-none-eabi-g++` + **linker script** (`.ld`) | Ghép các `.o`, gán địa chỉ thật: cái gì ở Flash, cái gì ở RAM |
| Chuyển định dạng | `arm-none-eabi-objcopy -O binary` | `.elf` (có ký hiệu, dùng để gỡ lỗi) → `.bin`/`.hex` (chỉ dữ liệu nạp vào chip) |
| Nạp | OpenOCD, `st-flash`, J-Link, `esptool`... | Ghi vào Flash qua SWD/JTAG/UART bootloader |

**Linker script** là nơi bạn khai báo bản đồ bộ nhớ của chip. Ví dụ tinh giản (chip giả định có 256 KB Flash tại `0x08000000` và 64 KB RAM tại `0x20000000`):

```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}
```

Phần còn lại của script ghép từng section vào vùng nhớ. Bạn hầu như không tự viết script từ đầu — hãng chip hoặc framework cung cấp — nhưng phải **đọc được** nó khi hết bộ nhớ.

### 2. Các section: biến của bạn nằm ở đâu

{{code:chapter-06/sections_demo.cpp}}

Quy tắc (chung cho hầu hết toolchain ELF):

| Loại biến | Section | Chiếm Flash? | Chiếm RAM? |
|-----------|---------|--------------|-----------|
| Mã lệnh | `.text` | Có | Không |
| Hằng (`const`, `constexpr`, chuỗi literal) | `.rodata` | Có | Thường không |
| Toàn cục/`static` có giá trị khởi tạo khác 0 | `.data` | Có (bản chép giá trị đầu) | Có |
| Toàn cục/`static` khởi tạo 0 hoặc không khởi tạo | `.bss` | Không | Có |
| Biến cục bộ | Stack | Không | Có (tạm) |
| `new`/`malloc` | Heap | Không | Có (tạm) |

Đo thử trên máy tác giả (GCC 16.2, Windows, `-Os`, chỉ biên dịch thành file đối tượng):

{{code:chapter-06/sections.size.txt}}

Đọc kết quả: `bss` = 288 byte gồm bộ đệm 256 byte và biến `calls`, cộng phần đệm căn lề; `data` = 16 chứa `g_counter`; `.rodata` (bảng `kTable`) được `size` gộp vào cột `text`. **Con số cụ thể khác nhau giữa định dạng file (Windows COFF khác ELF) và toolchain.** Điều bạn cần rút ra là *quy tắc phân loại*, và cách tự đo bằng `size` với đúng toolchain của chip mình.

Với chip thật: `arm-none-eabi-size firmware.elf` cho ba cột. **Flash cần dùng ≈ text + data. RAM tĩnh cần dùng ≈ data + bss.** Stack và heap cộng thêm.

### 3. Chuỗi khởi động (startup)
Khi chip reset, bộ xử lý đọc địa chỉ từ **bảng vector ngắt** và nhảy tới hàm `Reset_Handler`. Hàm này (bạn hoặc hãng viết, thường là assembly/C) làm theo thứ tự:

1. Đặt con trỏ stack.
2. Chép vùng `.data` từ Flash sang RAM (để biến có giá trị khởi tạo).
3. Xoá vùng `.bss` về 0.
4. (Tuỳ chọn) khởi tạo đồng hồ hệ thống.
5. Gọi các constructor của đối tượng toàn cục C++ (`__libc_init_array` hoặc tương tự).
6. Gọi `main()`.

Đây là lý do một đối tượng toàn cục C++ có constructor chạy **trước** `main`, và vì sao bước 5 có thể gây lỗi nếu bạn quên gọi trong startup tự viết (đối tượng toàn cục "không được khởi tạo").

### 4. Toolchain file CMake cho biên dịch chéo
Đoạn dưới **không biên dịch được trên PC** — nó minh hoạ cấu trúc cho chip Cortex-M4 (tên cờ CPU cần khớp với chip của bạn; xem tài liệu hãng).

```cmake
# arm-cortex-m4.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Không thử chạy chương trình thử của CMake trên PC
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS_INIT   "${CPU_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${CPU_FLAGS} -T${CMAKE_SOURCE_DIR}/linker.ld -Wl,--gc-sections -Wl,-Map=firmware.map --specs=nano.specs")
```

Dùng:

```bash
cmake -S . -B build-arm -G Ninja -DCMAKE_TOOLCHAIN_FILE=arm-cortex-m4.cmake
cmake --build build-arm
arm-none-eabi-size build-arm/firmware.elf
```

Ý nghĩa cờ đáng nhớ:
- `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections`: mỗi hàm/biến một section riêng để linker **loại bỏ** cái không dùng.
- `--specs=nano.specs`: dùng newlib-nano (thư viện C nhỏ gọn hơn); `printf` với số thực cần cờ riêng.
- `-fno-exceptions -fno-rtti`: bỏ hai tính năng tốn Flash (chương 3).
- `-Wl,-Map=...`: sinh bản đồ liên kết — công cụ số một khi hết Flash/RAM.

### 5. Gỡ lỗi trên chip thật

| Công cụ | Dùng khi | Ghi chú |
|---------|----------|---------|
| **SWD/JTAG + gdb** (qua OpenOCD, pyOCD hoặc J-Link GDB server) | Đặt breakpoint, xem biến, thanh ghi, stack | Nhiều board có sẵn mạch nạp/gỡ lỗi |
| **Log qua UART** | Chạy lâu, hành vi theo thời gian | Log chậm làm thay đổi thời gian; giữ log ngắn |
| **Logic analyzer** | Kiểm tra I²C/SPI/UART/GPIO thật sự phát ra gì | Rẻ và cực kỳ hiệu quả; xem chương 24–26 |
| **Oscilloscope** | Nguồn, nhiễu, dạng sóng | Cho lỗi "ma" liên quan nguồn/analog |
| **Mô phỏng (QEMU, Renode)** | Chạy firmware không cần board, tự động hoá CI | Không mô phỏng mọi ngoại vi; kiểm tra chip bạn dùng có được hỗ trợ không |

Lệnh gdb hay dùng (sau khi kết nối gdb server):

```text
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
(gdb) info registers
(gdb) x/16xw 0x20000000
(gdb) bt
```

**Hard fault.** Khi CPU thực thi điều không hợp lệ (truy cập địa chỉ sai, chia cho 0 nếu bật, lệnh không hợp lệ), Cortex-M nhảy vào `HardFault_Handler`. Cách xử lý thực tế: ở handler, lấy con trỏ stack và đọc **PC lúc lỗi** (thanh ghi được đẩy vào stack) rồi tra trong `.map`/`objdump` xem đó là dòng code nào. Các thanh ghi trạng thái lỗi (CFSR, HFSR) cho biết loại lỗi. Chi tiết theo từng chip nằm trong tài liệu ARM và hãng — nguyên tắc: **lưu lại PC/LR vào vùng nhớ giữ qua reset** để phân tích sau khi thiết bị tự khởi động lại.

## Ví dụ thực tiễn: "hết RAM" sau khi thêm một tính năng
Tình huống thường gặp: bạn thêm bộ đệm log 4 KB, build vẫn qua nhưng thiết bị treo sau vài phút. Cách điều tra:

1. `arm-none-eabi-size` — `data + bss` có gần bằng RAM chip không?
2. `nm --size-sort -S firmware.elf | tail` — ai to nhất?
3. Kiểm tra **stack**: RAM còn lại sau `data + bss` là chỗ dành cho stack và heap. Nếu chỉ còn vài trăm byte, hàm đệ quy hoặc mảng cục bộ lớn sẽ tràn stack và ghi đè biến toàn cục — triệu chứng "treo ngẫu nhiên".
4. Cách phòng: đặt "canary" ở đáy stack lúc khởi động (ghi mẫu `0xDEADBEEF`), định kỳ kiểm tra còn nguyên không.

## Góc nhúng
- Một build hợp lệ **chưa** nghĩa là chip đủ bộ nhớ: linker sẽ báo lỗi "region `RAM' overflowed" nếu vượt, nhưng **không** báo nếu stack thực tế lớn hơn phần dự trù.
- Bật `-Wl,--print-memory-usage` để linker in mức dùng Flash/RAM mỗi lần build (tuỳ phiên bản binutils).
- Đặt kích thước stack và heap **tường minh** trong linker script thay vì để mặc định.

## Lỗi thường gặp
- Quên `--gc-sections`: Flash phình vì code không dùng vẫn được giữ lại.
- Dùng `printf` với `%f` trên newlib-nano mà không bật hỗ trợ float: in ra trống hoặc sai.
- Debug bản `-O2`: biến bị tối ưu mất, breakpoint nhảy lung tung; dùng `-Og` hoặc `-O0` khi gỡ lỗi và **đo lại** với bản phát hành.
- Nạp firmware nhưng không reset chip: vẫn chạy bản cũ.
- Đặt đối tượng lớn có constructor phụ thuộc phần cứng ở phạm vi toàn cục (chạy trước `main`, trước khi đồng hồ được cấu hình).

## Bài tập
1. Biên dịch `sections_demo.cpp` với `-Os -c` rồi chạy `size`. Thêm một mảng toàn cục `std::uint8_t g_big[1000] = {1};` và so sánh cột `data` và `bss`. Giải thích.
2. Đổi `g_buffer` thành `const std::uint8_t g_buffer[256] = {}` (hằng). Cột nào thay đổi?
3. Dùng `g++ ... -Wl,-Map=demo.map` khi liên kết trên PC và tìm ký hiệu `g_counter` trong file map.
4. Viết toolchain file cho chip bạn đang dùng (điền cờ CPU đúng từ tài liệu), chưa cần build.
5. Viết một hàm đệ quy tính giai thừa; ước lượng mỗi lần gọi tốn bao nhiêu byte stack và giải thích vì sao đệ quy sâu nguy hiểm trên MCU.

## Tóm tắt
- Firmware = compile → link (với linker script) → objcopy → nạp; `.elf` để gỡ lỗi, `.bin/.hex` để nạp.
- `const` → Flash; biến có giá trị đầu → cả Flash và RAM (`.data`); biến bằng 0 → chỉ RAM (`.bss`).
- Trước `main` có chuỗi startup: đặt stack, chép `.data`, xoá `.bss`, chạy constructor toàn cục.
- Biên dịch chéo cần toolchain file, cờ CPU đúng, `--gc-sections` và tuỳ chọn `-fno-exceptions -fno-rtti`.
- Gỡ lỗi kết hợp gdb/SWD, log, logic analyzer; lưu dấu vết hard fault để phân tích sau.
