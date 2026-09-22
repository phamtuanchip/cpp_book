---
chapter: 26
title: "Máy trạng thái hữu hạn (FSM) cho firmware"
part: 6
code: code/chapter-26
---

# Chương 26. Máy trạng thái hữu hạn cho firmware

## Mục tiêu
- Viết FSM chống rung phím (debounce) bằng `enum class` + `switch`, gọi định kỳ, không dùng `delay()` chặn.
- Viết FSM phân tích khung giao thức byte-theo-byte, phù hợp dữ liệu đến từ ring buffer (chương 24).
- Nhận ra khi nào một mớ `if/else` lồng nhau thực chất là một máy trạng thái chưa được đặt tên — và vì sao đặt tên nó ra giúp code dễ suy luận hơn.

## Câu chuyện: `if (millis() - last > 50)` rải khắp nơi
Firmware ban đầu chống rung phím bằng: đọc chân, nếu khác lần trước thì `delay(50)` rồi đọc lại. `delay()` chặn toàn bộ chương trình — trong lúc đó UART không được xử lý, LED không nhấp nháy đúng nhịp. Thay vào đó, một FSM được gọi **định kỳ, không chặn** (từ vòng lặp chính hoặc timer) tự nhớ nó đang ở đâu trong quá trình chống rung, và chỉ báo "đã nhấn" khi tín hiệu ổn định đủ lâu — không có `delay()` nào.

## Kiến thức

### 1. FSM chống rung phím

{{code:chapter-26/button_fsm.cpp}}

{{out:chapter-26/button_fsm}}

Bốn trạng thái (`Released`, `Debouncing`, `Pressed`, `Releasing`) và một bộ đếm là toàn bộ trạng thái cần nhớ. `tick()` được gọi **mỗi chu kỳ cố định** (ví dụ mỗi 1 ms từ timer), không đợi, không `delay()`; các phần khác của firmware (UART, LED, SPI...) chạy song song bình thường giữa các lần gọi. Ví dụ cố tình đưa tín hiệu **rung** (nhiễu vài tick khi nhấn/thả) để chứng minh: FSM không báo "nhấn" cho tới khi tín hiệu ổn định đủ `kDebounceTicks` — đúng mục đích chống rung.

`consume_press_edge()`/`consume_release_edge()` trả về **cạnh** (sự kiện xảy ra đúng một lần) chứ không phải trạng thái hiện tại: vòng lặp chính đọc cạnh này để biết "vừa xảy ra sự kiện", tránh xử lý lặp lại cùng một lần nhấn nhiều lần nếu vòng lặp chính chạy chậm hơn `tick()`.

### 2. FSM phân tích khung giao thức

{{code:chapter-26/protocol_parser_fsm.cpp}}

{{out:chapter-26/protocol_parser_fsm}}

`FrameParser::feed(byte)` xử lý **từng byte một** — đúng cách dữ liệu đến từ ring buffer UART (chương 24): vòng lặp chính lấy từng byte ra khỏi ring buffer và gọi `feed`, không cần chờ đủ cả khung trong một buffer lớn duy nhất. Năm trạng thái (`WaitStx` → `WaitLen` → `ReadData` → `WaitChecksum` → `WaitEtx`) theo đúng cấu trúc khung `[STX][LEN][DATA...][CHECKSUM][ETX]`.

Ba tình huống trong ví dụ minh hoạ tính **chịu lỗi** của FSM:
- Khung hợp lệ → hoàn tất, in ra dữ liệu.
- Checksum sai → `reset` về `WaitStx`, không làm hỏng việc phân tích khung tiếp theo.
- Rác (`0xFF`, `0xEE`) đứng trước một khung hợp lệ → bị bỏ qua ở trạng thái `WaitStx` cho tới khi gặp đúng `kStx`, khung sau đó vẫn được phân tích đúng.

Đây là điểm khác biệt quan trọng so với việc parse cả khung một lần bằng `memcpy` vào một struct: nếu luồng byte bị lẫn rác hoặc mất đồng bộ (rất thường gặp trên UART thực tế do nhiễu, khởi động giữa chừng), FSM **tự phục hồi** ở byte tiếp theo, trong khi parse nguyên khối dễ đọc sai toàn bộ cấu trúc dữ liệu sau nó.

### 3. Khi nào một đống `if/else` là một FSM chưa đặt tên
Dấu hiệu: bạn cần một biến `bool`/`int` để "nhớ mình đang ở bước nào" giữa các lần gọi hàm, hoặc giữa các lần lặp. Nếu vậy, đặt tên rõ ràng cho từng trạng thái (`enum class`) và liệt kê rõ chuyển đổi trong `switch` gần như luôn dễ đọc và dễ kiểm tra đủ trường hợp hơn — trình biên dịch cảnh báo (`-Wswitch`) nếu bạn thêm trạng thái mới mà quên xử lý ở một `switch` nào đó.

## Góc nhúng
- `tick()`/`feed()` không cấp phát, không block — an toàn gọi từ ngữ cảnh thời gian thực hoặc thậm chí ISR nếu cần (dù xử lý khung giao thức phức tạp thường tốt hơn nếu để lại vòng lặp chính, xem chương 24 mục 2).
- Với FSM phức tạp hơn (nhiều chục trạng thái), cân nhắc `std::variant` (chương 17) khi mỗi trạng thái cần **dữ liệu khác nhau** đi kèm — `switch` trên `enum class` phù hợp khi mọi trạng thái dùng chung một bộ dữ liệu như hai ví dụ trên.
- Đo kích thước `sizeof(Button)`/`sizeof(FrameParser)`: toàn bộ trạng thái nằm trong vài byte, không heap — phù hợp làm thành viên tĩnh của driver.

## Lỗi thường gặp
- Chống rung phím bằng `delay()` chặn cả hệ thống thay vì FSM không chặn gọi định kỳ.
- Parser giao thức giả định luôn nhận đủ, đúng thứ tự byte — không có đường xử lý khi mất đồng bộ hoặc dữ liệu rác chen vào.
- `switch` trên `enum class` thiếu `default` **và** không xử lý hết mọi giá trị — một số trình biên dịch không cảnh báo nếu thêm giá trị enum mới sau này; luôn bật `-Wswitch` (mặc định có trong `-Wall`) và xử lý mọi nhánh tường minh thay vì dựa vào `default`.
- Trộn logic FSM với logic xuất I/O trực tiếp (gọi `gpio_write` ngay trong `switch`) khiến khó unit test FSM một mình trên host (chương 29 quay lại điểm này).

## Bài tập
1. Thêm trạng thái `LongPress` vào `Button`: nếu giữ nút quá 1000 tick, phát sinh một cạnh riêng `on_long_press_edge_`.
2. Sửa `FrameParser` để hỗ trợ `LEN = 0` (khung không có dữ liệu) — kiểm tra code hiện tại đã xử lý đúng trường hợp này chưa (gợi ý: xem nhánh `WaitLen`).
3. Viết một bộ test thủ công (không cần framework) gọi `feed()` với một khung bị **cắt cụt** giữa chừng (dữ liệu kết thúc đột ngột) — parser có kẹt ở trạng thái giữa chừng mãi không, và nên xử lý thế nào (gợi ý: timeout dựa trên số tick không nhận thêm byte)?
4. Giải thích bằng lời vì sao FSM byte-theo-byte phù hợp hơn với dữ liệu từ ring buffer UART so với việc chờ đủ N byte rồi `memcpy` một lần.

## Tóm tắt
- FSM gọi định kỳ, không chặn, là cách chuẩn để chống rung phím và các tác vụ "chờ ổn định" khác trong firmware thời gian thực.
- FSM byte-theo-byte cho phân tích giao thức tự nhiên khớp với luồng dữ liệu từ ring buffer, và tự phục hồi khi gặp rác/mất đồng bộ.
- Khi thấy mình cần một biến "nhớ trạng thái" giữa các lần gọi, đó là dấu hiệu nên viết một FSM có tên rõ ràng thay vì `if/else` rải rác.
