---
chapter: 29
title: "Unit test host-based và giả lập phần cứng"
part: 7
code: code/chapter-29
---

# Chương 29. Unit test host-based và giả lập phần cứng

## Mục tiêu
- Viết unit test cho logic thuần (FSM, thuật toán) chạy trên host, không cần MCU/board.
- Kiểm thử driver phụ thuộc phần cứng bằng cách tiêm (inject) một bus/GPIO giả lập, tận dụng HAL kiểu template (chương 25).
- Biết vì sao tách "logic" khỏi "I/O trực tiếp" là điều kiện tiên quyết để test được, và cách các framework thật (Catch2, doctest, GoogleTest) khác gì bản tối giản trong chương.

## Câu chuyện: bug chỉ tái hiện được trên board thật
Một lỗi trong FSM chống rung phím (chương 26) chỉ xuất hiện với một chuỗi tín hiệu hiếm gặp (rung đúng vào thời điểm biên bộ đếm). Tìm lỗi bằng cách nhấn nút thật trên board hàng chục lần là chuyện may rủi. Nếu `Button` không phụ thuộc gì vào GPIO thật — chỉ nhận `bool` đầu vào và trả `bool` đầu ra — nó có thể được test bằng cách "phát lại" chính xác chuỗi tín hiệu gây lỗi, trên máy phát triển, chạy trong mili giây, lặp lại được vô hạn lần.

## Kiến thức

### 1. Test logic thuần, không cần framework ngoài

{{code:chapter-29/mini_test_framework.cpp}}

{{out:chapter-29/mini_test_framework}}

`Button` ở đây giống hệt tinh thần chương 26: không `#include` gì về GPIO, chỉ nhận `bool raw_pressed` và trả cạnh sự kiện. Vì vậy nó **test được trực tiếp** bằng vài dòng macro tối giản (`CHECK`), không cần thư viện ngoài, không cần mock gì — đây là lợi ích lớn nhất của việc tách logic khỏi phần cứng.

Ba bài test minh hoạ ba **kịch bản**, không phải ba dòng code ngẫu nhiên:
- Không bao giờ nhận → không bao giờ chuyển sang `Pressed`.
- Nhận đủ lâu → chuyển sang `Pressed`, và cạnh sự kiện chỉ xuất hiện **đúng một lần** tại thời điểm chuyển.
- Rung ngắn (nhả trước khi đủ debounce) → không được tính là một lần nhấn.

Mỗi bài test độc lập (tạo `Button b` mới), không phụ thuộc thứ tự chạy — nguyên tắc quan trọng để test đáng tin cậy.

### 2. Test driver phụ thuộc phần cứng bằng bus giả lập

{{code:chapter-29/hardware_mock_test.cpp}}

{{out:chapter-29/hardware_mock_test}}

`ThermalGuard<Bus>` (cùng mẫu HAL kiểu `concept` ở chương 25) không quan tâm `Bus` là I2C thật hay giả lập. `MockBus` đóng hai vai trò kinh điển của một "test double":
- **Stub**: `program_read_value` lập trình sẵn giá trị trả về, để test kiểm soát hoàn toàn kịch bản (nhiệt độ 50 rồi 95) mà không cần cảm biến thật nóng lên.
- **Spy**: `last_read_addr()`/`read_count()` ghi lại **cách** driver đã gọi bus, để test xác nhận driver đọc đúng địa chỉ/thanh ghi — bắt được lỗi kiểu "đọc nhầm địa chỉ nhưng tình cờ vẫn ra kết quả đúng" mà chỉ kiểm tra giá trị trả về không phát hiện được.

Vì `ThermalGuard` nhận `Bus&` qua constructor thay vì tự tạo bus bên trong, nó **có thể** nhận bất kỳ kiểu nào thoả `concept I2cBus` — đây chính là "dependency injection" (tiêm phụ thuộc), không cần framework, chỉ cần thiết kế API nhận phụ thuộc từ bên ngoài thay vì tự khởi tạo bên trong.

### 3. Framework thật: Catch2/doctest/GoogleTest
Bản `CHECK`/`run_test` trong chương chỉ có vài dòng, đủ để dạy khái niệm. Trong dự án thật, một framework như **Catch2** hay **doctest** cho: gom nhóm test theo file, chạy chọn lọc theo tên/tag, so sánh giá trị in ra đẹp khi thất bại, đo thời gian, tích hợp CI. Cấu trúc thường gặp (tham khảo, kiểm tra đúng phiên bản trong tài liệu chính thức của framework bạn chọn):

```cpp
TEST_CASE("Button debounce bo qua rung ngan") {
    Button b;
    REQUIRE(b.tick(true) == false);
    REQUIRE(b.tick(false) == false);
    REQUIRE(b.is_pressed() == false);
}
```

Thêm framework này vào dự án nhúng thường qua CMake (`FetchContent` hoặc submodule), biên dịch và **chạy trên host** (x86/x64), tách biệt hoàn toàn khỏi bản build cho MCU thật — đây là ý nghĩa của "host-based testing" trong tiêu đề chương.

## Góc nhúng
- Test host chạy trong CI (GitHub Actions...) mỗi lần commit — bắt lỗi logic **trước khi** nạp lên board, rẻ hơn rất nhiều so với debug trên phần cứng.
- Không phải mọi thứ test được trên host: timing chính xác, điện áp thực tế, nhiễu điện từ... vẫn cần board thật. Unit test host-based bổ sung, không thay thế, kiểm thử tích hợp trên phần cứng.
- Driver càng tách rõ "quyết định" (logic) khỏi "hành động" (ghi thanh ghi), càng dễ test — đây là lý do thiết kế HAL ở chương 25 và FSM thuần ở chương 26 không chỉ đẹp về mặt kiến trúc mà còn **thực dụng** cho việc test.

## Lỗi thường gặp
- Viết driver gọi thẳng địa chỉ MMIO cố định bên trong logic nghiệp vụ, không qua tham số/interface — không thể thay thế bằng giả lập để test.
- Test phụ thuộc thứ tự chạy (test sau dựa vào trạng thái global bị test trước thay đổi) — thất bại ngẫu nhiên khi đổi thứ tự hoặc chạy song song.
- Chỉ test "đường vui" (giá trị hợp lệ), bỏ qua biên và trường hợp lỗi (bus trả giá trị bất thường, timeout) — thường chính là nơi bug thật xảy ra.
- Nhầm mock **thay thế hoàn toàn** hành vi cần test — ví dụ mock trả `true` vô điều kiện cho `is_overheating()` thay vì mô phỏng đúng bus, khiến test không còn kiểm tra được gì có ý nghĩa.

## Bài tập
1. Thêm bài test cho `Button`: nhấn, thả đúng lúc đạt ngưỡng debounce (không sớm hơn, không muộn hơn) — kiểm tra biên chính xác của bộ đếm.
2. Thêm phương thức `MockBus::fail_next_read()` khiến lần đọc tiếp theo trả về một giá trị đặc biệt (ví dụ `0xFF`, thường là dấu hiệu lỗi bus thật); viết test cho `ThermalGuard` xử lý tình huống đó (hiện tại nó coi `0xFF` là nhiệt độ hợp lệ — có nên sửa không?).
3. Viết một bài test cố tình **thất bại** (sửa `CHECK` với điều kiện sai) để quan sát định dạng thông báo lỗi, rồi sửa lại đúng.
4. Nếu có sẵn CMake và mạng, thử thêm Catch2 qua `FetchContent` vào một dự án con nhỏ và viết lại một trong các test của chương bằng `TEST_CASE`/`REQUIRE` — so sánh trải nghiệm với bản `CHECK` tối giản.

## Tóm tắt
- Logic tách khỏi I/O trực tiếp (FSM thuần, HAL qua interface/template) là điều kiện để test được trên host mà không cần phần cứng.
- Bus/GPIO giả lập (test double) đóng vai stub (trả giá trị lập trình sẵn) và spy (ghi lại lời gọi) để kiểm tra cả kết quả lẫn cách driver hoạt động.
- Unit test host-based chạy nhanh, lặp lại được, hợp với CI — bổ sung cho, không thay thế, kiểm thử tích hợp trên board thật.
