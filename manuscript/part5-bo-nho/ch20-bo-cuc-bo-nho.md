---
chapter: 20
title: "Bố cục bộ nhớ chương trình: .text, .data, .bss, stack, heap"
part: 5
code: code/chapter-20
---

# Chương 20. Bố cục bộ nhớ chương trình

## Mục tiêu
- Biết chương trình C++ được chia thành những vùng bộ nhớ nào, và mỗi vùng nằm ở đâu trên MCU (Flash hay RAM).
- Hiểu vì sao nhiều dự án nhúng **tắt hẳn heap**, và stack overflow nguy hiểm khác gì so với PC.
- Biết cách ước lượng mức dùng stack và đọc báo cáo kích thước bộ nhớ của linker.

## Câu chuyện: firmware chạy tốt trên bàn, "treo" khi thêm một tính năng nhỏ
Một firmware chạy ổn định nhiều tháng. Thêm một hàm log ghi chuỗi vào buffer cục bộ 512 byte, thiết bị bắt đầu treo ngẫu nhiên. Nguyên nhân: MCU chỉ có 8 KB RAM, ngăn xếp (stack) và các biến toàn cục **dùng chung một vùng RAM**, không có MMU bảo vệ. Buffer 512 byte đẩy đỉnh stack đè lên vùng `.bss` chứa con trỏ quan trọng — chương trình vẫn "chạy", chỉ là dữ liệu đã hỏng âm thầm. Hiểu bố cục bộ nhớ giúp bạn thấy nguy cơ này **trước khi** nó xảy ra trên bàn thử.

## Kiến thức

### 1. Các vùng bộ nhớ

{{code:chapter-20/memory_regions.cpp}}

{{out:chapter-20/memory_regions}}

| Vùng | Chứa gì | Trên MCU thường nằm ở | Cấp phát/giải phóng |
|------|---------|------------------------|----------------------|
| `.text` | Mã máy (hàm) | Flash | Cố định, ghi lúc nạp firmware |
| `.rodata` | Hằng số (`const`, chuỗi literal) | Flash | Cố định |
| `.data` | Biến tĩnh/toàn cục **có** giá trị khởi tạo khác 0 | Giá trị ban đầu ở Flash, **sao chép sang RAM** lúc khởi động | Tồn tại suốt chương trình |
| `.bss` | Biến tĩnh/toàn cục khởi tạo **bằng 0** (hoặc không khởi tạo) | Chỉ RAM (không tốn Flash) | RAM được dọn về 0 lúc khởi động |
| Stack | Biến cục bộ, tham số hàm, địa chỉ trả về | RAM, thường ở **đầu cao** của vùng RAM, lớn dần xuống | Tự động theo lời gọi hàm |
| Heap | Vùng cấp phát động (`new`/`malloc`) | RAM, thường ở **đầu thấp**, lớn dần lên (đối đầu với stack) | Thủ công (`new`/`delete`) |

Trên PC, hệ điều hành cấp mỗi tiến trình một không gian địa chỉ riêng và trang bộ nhớ theo yêu cầu; trên MCU chạy trực tiếp (bare-metal), **toàn bộ RAM vật lý được chia sẵn lúc link** theo linker script, và không có ai ngăn stack đè lên heap hay `.bss` — đó là lý do bug ở trên "chạy được nhưng sai".

Vì sao `.data` cần "sao chép từ Flash sang RAM lúc khởi động"? Vì Flash không ghi được nhanh/tuỳ ý lúc chạy — biến có thể thay đổi phải nằm ở RAM, nhưng **giá trị khởi tạo ban đầu** phải được lưu ở đâu đó bền (Flash) để nạp lại mỗi lần bật nguồn. Đoạn code khởi động (`startup.s`/`Reset_Handler`, chương 6) làm việc này trước khi gọi `main`; `.bss` thì chỉ cần dọn về 0, không cần sao chép gì.

### 2. Stack: ai dùng, dùng bao nhiêu

{{code:chapter-20/stack_probe.cpp}}

{{out:chapter-20/stack_probe}}

Mỗi lời gọi hàm đẩy một **khung stack** (địa chỉ trả về, biến cục bộ, tham số) và gỡ ra khi hàm kết thúc — không cần bạn quản lý, đó là lý do RAII (chương 12) hoạt động tự động. Vấn đề trên MCU:
- Không có bảo vệ phần cứng (guard page) như PC: stack tràn xuống đè lên vùng khác **âm thầm**, không có lỗi "segmentation fault" để báo ngay.
- Đệ quy sâu, mảng cục bộ lớn, hoặc lồng ngắt sâu là ba nguyên nhân phổ biến nhất gây tràn stack trên MCU.
- Kỹ thuật đo thực tế: **watermarking** — điền một mẫu (ví dụ `0xAA`) vào toàn bộ vùng stack lúc khởi động, sau đó định kỳ quét từ đáy lên để tìm byte đầu tiên còn nguyên mẫu; khoảng đó là phần **chưa bao giờ dùng tới**. RTOS như FreeRTOS cung cấp sẵn (`uxTaskGetStackHighWaterMark`) cho từng task.

### 3. Vì sao nhiều firmware tắt hẳn heap
- **Phân mảnh (fragmentation)**: cấp phát/giải phóng nhiều kích thước khác nhau theo thời gian để lại các lỗ trống không dùng được, dù tổng bộ nhớ trống vẫn đủ.
- **Không dự đoán được thời gian**: `malloc`/`new` có thể mất thời gian thay đổi tuỳ tình trạng heap — xấu cho code thời gian thực.
- **Hết bộ nhớ giữa chừng**: `new` thất bại giữa lúc chạy khó xử lý đúng trên hệ thống không cho phép dừng; với `-fno-exceptions`, `new` thất bại gọi `std::terminate` thay vì ném ngoại lệ.

Nhiều dự án chọn: cấp phát **tĩnh** mọi thứ biết trước lúc biên dịch (mảng cố định, pool cố định — chương 22), hoặc cấp phát động **chỉ một lần lúc khởi động** rồi không bao giờ `delete`/`new` lại nữa (chấp nhận được vì không còn rủi ro phân mảnh sau đó).

## Góc nhúng
- Đọc báo cáo kích thước bằng `arm-none-eabi-size firmware.elf`: cột `text` (Flash: code + rodata), `data` (Flash **và** RAM, vì cần cả bản gốc lẫn bản sao), `bss` (chỉ RAM). Flash dùng = text + data; RAM tĩnh dùng = data + bss (chưa tính stack/heap).
- Xem file `.map` (tạo bằng `-Wl,-Map=firmware.map`) để biết ký hiệu nào chiếm bao nhiêu và nằm ở địa chỉ nào — hữu ích khi cần cắt giảm.
- Đặt kích thước stack trong linker script (`_Min_Stack_Size` hoặc tương tự) dựa trên đo đạc thực tế cộng biên an toàn, không phải đoán.

## Lỗi thường gặp
- Khai báo mảng cục bộ lớn (`uint8_t buf[4096]`) trong một hàm gọi từ ISR — ISR có stack riêng thường **nhỏ hơn** stack chính.
- Tưởng `.bss` "tốn Flash" vì biến toàn cục — thực ra `.bss` không tốn Flash, chỉ tốn RAM và một dòng lệnh dọn về 0 lúc khởi động.
- Đệ quy không có điều kiện dừng chắc chắn trên dữ liệu từ bên ngoài (dễ bị tấn công bằng input gây đệ quy sâu).
- Không đặt kích thước stack cho từng task RTOS dựa trên đo đạc, dùng một con số mặc định cho mọi task dù nhu cầu rất khác nhau.

## Bài tập
1. Dùng `arm-none-eabi-size` (hoặc `size` trên host) với một chương trình bất kỳ trong `code/`; đối chiếu cột `text`/`data`/`bss` với bảng ở mục 1.
2. Sửa `stack_probe.cpp` để in thêm địa chỉ ở độ sâu 40 và so sánh byte/khung với độ sâu 20 — có nhất quán không?
3. Viết một watermark đơn giản: một mảng `static uint8_t stack_area[256]` điền `0xAA`, sau đó "dùng" một phần bằng cách ghi vào từ đầu, rồi quét tìm byte `0xAA` đầu tiên còn sót — đây là bản rút gọn của kỹ thuật thật, không phải đo stack CPU thật.
4. Giải thích vì sao một hàm đệ quy đuôi (tail recursion) *có thể* không tốn thêm stack nếu trình biên dịch tối ưu — và vì sao **không nên dựa vào** điều đó cho code an toàn (tối ưu này không được chuẩn C++ đảm bảo).

## Tóm tắt
- Chương trình chia thành `.text`/`.rodata` (Flash), `.data`/`.bss` (RAM, có/không cần sao chép từ Flash), stack và heap (RAM, cấp phát tự động/thủ công).
- MCU không có bảo vệ phần cứng giữa các vùng: tràn stack đè lên dữ liệu khác một cách âm thầm.
- Đo bằng `size`/file `.map`/watermarking thay vì đoán; nhiều firmware tắt hẳn heap để có bộ nhớ dự đoán được.
