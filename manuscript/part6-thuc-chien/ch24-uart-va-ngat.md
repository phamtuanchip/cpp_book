---
chapter: 24
title: "UART, ring buffer và ngắt (ISR) an toàn"
part: 6
code: code/chapter-24
---

# Chương 24. UART, ring buffer và ngắt (ISR) an toàn

## Mục tiêu
- Viết ring buffer không khoá (lock-free) an toàn cho đúng một ISR ghi, một vòng lặp chính đọc.
- Biết các ràng buộc bắt buộc của code chạy trong ISR: ngắn, không heap, không block, không ném ngoại lệ.
- Nối một ISR (yêu cầu C linkage, tên cố định) với driver viết bằng C++.

## Câu chuyện: mất byte UART khi tải cao
Driver UART ban đầu chỉ lưu **byte cuối cùng nhận được** vào một biến `volatile`. Khi tốc độ baud tăng và vòng lặp chính bận xử lý việc khác, các byte đến dồn dập bị **ghi đè lên nhau** trước khi vòng lặp chính kịp đọc — mất dữ liệu mà không có cảnh báo nào. Giải pháp chuẩn: một **hàng đợi vòng (ring buffer)** giữa ISR (người sản xuất) và vòng lặp chính (người tiêu thụ), đủ lớn để hấp thụ độ trễ xử lý.

## Kiến thức

### 1. Ring buffer lock-free cho một ISR, một vòng lặp chính

{{code:chapter-24/uart_ring_buffer.cpp}}

{{out:chapter-24/uart_ring_buffer}}

Với đúng **một** người ghi (`push`, gọi từ ISR) và **một** người đọc (`pop`, gọi từ vòng lặp chính), không cần mutex hay tắt ngắt: hai chỉ số `head`/`tail` độc lập, mỗi bên chỉ sửa chỉ số của mình và chỉ đọc chỉ số của bên kia.
- `head_.store(..., memory_order_release)` sau khi ghi dữ liệu vào `buf_`, và `head_.load(..., memory_order_acquire)` trước khi đọc dữ liệu ở bên đọc: cặp release/acquire đảm bảo bên đọc **thấy được** dữ liệu đã ghi, không chỉ thấy chỉ số đã cập nhật (nếu không, trình biên dịch hoặc CPU có thể sắp xếp lại, khiến bên đọc thấy `head` mới nhưng dữ liệu cũ).
- `N` là luỹ thừa của 2 để dùng `& (N-1)` thay cho `% N` — phép AND rẻ hơn chia lấy dư trên hầu hết MCU, `static_assert` bắt lỗi ngay nếu ai đó đổi `N` thành số không phải luỹ thừa 2.
- Buffer đầy: `push` trả `false` thay vì ghi đè hay chờ — **mất byte có kiểm soát** (với cảnh báo) luôn tốt hơn treo ISR hoặc ghi đè dữ liệu chưa đọc.

### 2. Quy tắc bắt buộc trong ISR
1. **Ngắn nhất có thể**: ISR càng dài, càng trễ các ngắt khác (hoặc chính nó nếu ngắt lồng bị cấm). Đẩy dữ liệu vào ring buffer rồi thoát ngay; xử lý nặng để lại cho vòng lặp chính.
2. **Không cấp phát heap**: `new`/`malloc` không xác định thời gian và có thể không an toàn khi bị ngắt giữa chừng bởi ISR khác (chương 20, 22).
3. **Không ném ngoại lệ**: nhiều toolchain nhúng build `-fno-exceptions`; kể cả khi bật, unwind ngoại lệ trong ISR thường không được hỗ trợ đúng.
4. **Biến chia sẻ phải `volatile`/`atomic`** đúng như chương 21 — thiếu, tối ưu hoá có thể khiến vòng lặp chính không bao giờ thấy dữ liệu mới.
5. **Không gọi hàm chặn** (chờ semaphore kiểu blocking, `printf` ra UART tốc độ thấp...) — ISR không có "thời gian chờ", nó phải trả quyền điều khiển lại cho phần cứng.

### 3. Nối ISR (C linkage) với driver C++

{{code:chapter-24/extern_c_isr.cpp}}

{{out:chapter-24/extern_c_isr}}

Startup file của vendor (thường viết bằng assembly hoặc C, sinh bởi CMSIS/CubeMX) chứa **bảng vector ngắt** trỏ tới các hàm theo **tên cố định** (ví dụ `USART1_IRQHandler`). C++ đổi tên hàm khi biên dịch (name mangling, chi tiết ở chương 28) để hỗ trợ nạp chồng — nếu khai báo ISR như một hàm C++ bình thường, tên sau biên dịch sẽ không khớp với tên trong bảng vector, và ngắt sẽ **không bao giờ được gọi** (hoặc rơi vào handler mặc định) mà trình biên dịch không báo lỗi gì. `extern "C"` buộc giữ nguyên tên, trong khi thân hàm vẫn là C++ bình thường — được phép gọi phương thức của đối tượng C++ (như `g_uart1.on_rx_isr(...)`).

Đối tượng driver (`g_uart1`) phải có **thời gian sống toàn cục** (static/global): ISR không có ngữ cảnh "gọi hàm với tham số driver" — nó được phần cứng gọi thẳng, không tham số, nên phải biết trước đối tượng nào để thao tác.

## Góc nhúng
- Đo thời gian thực thi ISR bằng cách bật một chân GPIO (chương 23) ở đầu ISR và tắt ở cuối, quan sát bằng oscilloscope/logic analyzer — cách đo thực tế thay vì đoán.
- Kích thước ring buffer đánh đổi RAM lấy khả năng chịu trễ xử lý; tính toán dựa trên tốc độ baud và thời gian xử lý tệ nhất của vòng lặp chính, không chọn số tuỳ tiện.
- Tên hàm ISR chính xác (`USART1_IRQHandler`, `EXTI0_IRQHandler`...) nằm trong file khởi động của vendor SDK (CMSIS) — luôn tra đúng tên cho MCU cụ thể, sách này không thể liệt kê hết cho mọi dòng chip.

## Lỗi thường gặp
- Quên `extern "C"` trên hàm ISR — trình biên dịch build thành công (nó chỉ là một hàm C++ bình thường "mồ côi", không ai gọi), nhưng ngắt không bao giờ chạy, và lỗi này **khó phát hiện** vì không có thông báo nào.
- Gọi hàm có thể block (chờ mutex, gửi UART đồng bộ tốc độ thấp) bên trong ISR.
- Dùng ring buffer nhiều-người-ghi hoặc nhiều-người-đọc mà không thêm khoá — thiết kế trong chương này **chỉ đúng** cho một ghi/một đọc.
- Không kiểm tra giá trị trả về của `push`, âm thầm mất dữ liệu mà không log/đếm để chẩn đoán sau này.

## Bài tập
1. Thêm bộ đếm `dropped_count` vào `RingBuffer`, tăng mỗi khi `push` thất bại; in ra trong `main`.
2. Viết phiên bản `pop_all` đọc hết dữ liệu hiện có vào một `std::array` (dùng cho vòng lặp chính xử lý theo lô thay vì từng byte).
3. Giải thích vì sao `RingBuffer<T, N>` trong chương này **không an toàn** nếu có hai ISR khác nhau cùng gọi `push` (ví dụ UART1 và UART2 dùng chung một buffer) — và nêu cách sửa tối thiểu.
4. Tìm trong tài liệu CMSIS/reference manual của một MCU bất kỳ (STM32, nRF52...) tên chính xác của ISR nhận dữ liệu UART, đối chiếu với ví dụ trong chương.

## Tóm tắt
- Ring buffer một-ghi/một-đọc với `std::atomic` + `memory_order` đúng cặp là mẫu chuẩn để trao dữ liệu giữa ISR và vòng lặp chính mà không cần khoá.
- ISR phải ngắn, không heap, không ngoại lệ, không block; xử lý nặng luôn để lại cho vòng lặp chính.
- Hàm ISR cần `extern "C"` để giữ đúng tên mà bảng vector ngắt của startup file mong đợi.
