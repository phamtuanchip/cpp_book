---
chapter: 8
title: "Hàm, luồng điều khiển và xử lý lỗi cơ bản"
part: 2
code: code/chapter-08
---

# Chương 8. Hàm, luồng điều khiển và xử lý lỗi cơ bản

## Mục tiêu
- Viết hàm rõ ràng: đối số mặc định, nạp chồng, `inline`, `[[nodiscard]]`.
- Biểu diễn mã lỗi bằng `enum class` thay cho số "ma thuật".
- Viết logic điều khiển không chặn (non-blocking) dựa trên trạng thái, thay vì `delay`.
- Xây một bộ chống dội nút bấm — bài toán có mặt trong hầu hết sản phẩm.

## Câu chuyện: nút bấm "tự bấm hai lần"
Một công tắc cảm ứng cho đèn thông minh: khách hàng phàn nàn bấm một lần đèn bật rồi tắt ngay. Nguyên nhân: tiếp điểm cơ khí **dội** (bounce) — trong vài mili-giây tín hiệu nhảy 0-1-0-1 trước khi ổn định; firmware coi mỗi lần nhảy là một lần bấm. Giải pháp là chống dội phần mềm — và cũng là cơ hội học cách viết hàm/lớp nhỏ, kiểm thử được.

## Kiến thức

### 1. Hàm nhỏ, tên rõ, kiểu rõ
Nguyên tắc cho firmware:
- Mỗi hàm một việc; đặt tên theo hành động (`uart_send`, `sensor_read`).
- Trả về **trạng thái** (`Status`) khi có thể thất bại; không dùng `-1`, `0xFF` "ma thuật".
- Tham số đầu vào lớn: `const T&`. Tham số ra: tham chiếu không-const hoặc trả về struct (xem chương 9).
- Không dùng biến toàn cục để "truyền tin" giữa các hàm nếu tránh được.

### 2. Nạp chồng, đối số mặc định, `[[nodiscard]]`

{{code:chapter-08/status_overload.cpp}}

{{out:chapter-08/status_overload}}

- **Nạp chồng (overloading):** cùng tên `log`, khác kiểu đối số. Trình biên dịch chọn lúc biên dịch — không tốn thời gian chạy.
- **Đối số mặc định:** `timeout_ms = 100` để lời gọi thường gặp ngắn gọn. Chỉ đặt giá trị mặc định ở khai báo (header), một lần.
- **`enum class Status : std::uint8_t`:** mã lỗi có kiểu, có phạm vi (`Status::Timeout`), chiếm 1 byte.
- **`[[nodiscard]]`:** nếu người gọi bỏ quên kết quả, trình biên dịch cảnh báo. Với các hàm có thể thất bại (gửi, ghi Flash, đọc cảm biến), đây là lưới an toàn rẻ nhất: lỗi bị bỏ qua là nguồn của nhiều sự cố hiện trường.

### 3. `inline`, `constexpr` và `static`
- `inline`: cho phép định nghĩa hàm trong header mà không vi phạm quy tắc một-định-nghĩa; trình biên dịch tự quyết có nhúng vào nơi gọi hay không. Với `-O2` hàm nhỏ thường được nhúng.
- `constexpr`: hàm có thể chạy lúc biên dịch (ví dụ `to_string` ở trên dùng được trong `static_assert`).
- `static` ở phạm vi file: hàm/biến chỉ thấy trong file `.cpp` đó (tránh xung đột tên khi liên kết); trong C++ ưa dùng **namespace vô danh** `namespace { ... }` cho cùng mục đích.

### 4. Luồng điều khiển: `if`, `switch`, vòng lặp
- Dùng `switch` trên `enum class` **không có nhánh `default`** để trình biên dịch cảnh báo khi bạn thêm giá trị mới mà quên xử lý (`-Wswitch`). Ví dụ `to_string` ở trên.
- Vòng lặp `for (auto x : container)` (range-based for) an toàn hơn chỉ số thủ công.
- Tránh vòng lặp vô tận chờ điều kiện phần cứng mà không có **thời gian chờ tối đa**: nếu chip ngoại vi hỏng, thiết bị treo mãi. Luôn có timeout, hoặc để watchdog cứu.

### 5. Không chặn: thay `delay()` bằng trạng thái
`delay_ms(500)` dừng cả chương trình. Cách thay: lưu trạng thái, gọi hàm cập nhật **định kỳ**, mỗi lần chỉ làm một chút rồi trả lại điều khiển. Ví dụ tiêu biểu dưới đây.

## Ví dụ thực tiễn: chống dội nút bấm

{{code:chapter-08/debounce.cpp}}

{{out:chapter-08/debounce}}

Cách hoạt động: nút chỉ đổi trạng thái ổn định khi đọc được **4 mẫu liên tiếp** khác với trạng thái hiện tại. Chuỗi thử có nhiễu ở đầu (`0 1 0 1 1 0 1...`): ba lần dội đầu bị bỏ qua, chỉ đến `t=9` (sau bốn mẫu `1` liên tiếp) mới báo "NHẤN". Lần nhả cũng cần bốn mẫu `0` liên tiếp (`t=17`); cú nhả nhiễu `0 1 0` ở `t=12–14` không làm đổi trạng thái.

Điểm thiết kế đáng học:
- Lớp `Debouncer` **không biết phần cứng**: chỉ nhận `bool raw`. Nhờ vậy test trên PC như trên.
- `update()` được gọi định kỳ (ví dụ mỗi 1 ms từ ngắt timer, hoặc mỗi 5 ms từ vòng lặp chính); số mẫu ổn định × chu kỳ = thời gian chống dội. 4 mẫu × 1 ms = 4 ms là mức thường dùng.
- Hàm trả `true` khi trạng thái vừa **đổi** → phần còn lại của chương trình phản ứng theo sự kiện, không phải thăm dò liên tục.

## Góc nhúng
- Đối số mặc định và nạp chồng không tốn gì lúc chạy. Nhưng **mỗi hàm inline lớn được nhúng nhiều nơi** làm tăng Flash; quan sát bằng `size`.
- Gọi hàm có `std::printf` trong ngắt là ý tưởng tồi: chậm, có thể chặn. Ngắt chỉ nên đặt cờ hoặc đẩy dữ liệu vào hàng đợi (chương 27).
- `Debouncer` chiếm đúng 3 byte + đệm căn lề; kiểm tra bằng `sizeof`.

## Lỗi thường gặp
- Bỏ qua giá trị trả về của hàm ghi Flash/gửi UART: lỗi chỉ lộ ra khi thiết bị ngoài hiện trường.
- Đặt `default:` trong `switch` trên `enum class`, rồi thêm giá trị mới mà không được cảnh báo.
- Vòng lặp `while (!(REG & READY))` không có timeout.
- Dùng `delay` trong hàm được gọi từ nhiều nơi; các tác vụ khác bị đói.
- Chống dội bằng `delay(20)` trong ngắt: chặn cả hệ thống 20 ms.

## Bài tập
1. Thêm `Status::Busy` vào `uart_send` khi biến toàn cục `g_uart_busy == true`. Thêm ca kiểm thử trong `main`.
2. Bỏ comment dòng `uart_send(msg, 2);` không dùng kết quả. Chép cảnh báo của trình biên dịch.
3. Sửa `Debouncer` để nhận **hai** ngưỡng khác nhau (số mẫu để nhấn, số mẫu để nhả).
4. Thêm phát hiện **nhấn giữ**: sau khi nhấn ổn định 1000 mẫu (1 s) trả về sự kiện `LongPress`. Thiết kế `enum class Event { None, Pressed, Released, LongPress }`.
5. Viết `blink` không chặn: hàm `update(now_ms)` đảo LED mỗi 500 ms, dùng phép trừ thời gian đúng như chương 7.

## Tóm tắt
- Hàm nhỏ, trả `Status` tường minh (`enum class`), `[[nodiscard]]` cho hàm có thể lỗi.
- Nạp chồng và đối số mặc định giải quyết lúc biên dịch, không tốn phí.
- `switch` trên `enum class` không có `default` để được cảnh báo khi thiếu ca.
- Thay `delay` bằng trạng thái + cập nhật định kỳ; luôn có timeout khi chờ phần cứng.
- Logic độc lập phần cứng (như `Debouncer`) test được trên PC.
