---
chapter: 13
title: "Kế thừa và tổ hợp: is-a hay has-a"
part: 3
code: code/chapter-13
---

# Chương 13. Kế thừa và tổ hợp: is-a hay has-a

## Mục tiêu
- Phân biệt quan hệ "is-a" (kế thừa đúng chỗ) và "has-a" (tổ hợp) bằng ví dụ phần cứng cụ thể.
- Nhận ra dấu hiệu kế thừa bị dùng sai — nguyên nhân phổ biến của thiết kế cứng nhắc.
- Biết quy tắc thực dụng: trong nhúng, **ưu tiên tổ hợp**, chỉ kế thừa khi thật sự có quan hệ "là một loại của".

## Câu chuyện: cảm biến "là một" cái bus?
Một lập trình viên mới viết `class TempSensor : public I2cBus` vì "gõ nhanh hơn, không cần viết lại các hàm đọc/ghi thanh ghi". Vài tuần sau, dự án cần **hai cảm biến dùng chung một bus vật lý** (đúng như board thật: một bus I2C, nhiều thiết bị trên đó). Nhưng mỗi `TempSensor` kế thừa lại tự chứa một `I2cBus` **riêng** — không có cách nào biểu diễn "hai đối tượng cùng dùng một bus" bằng kế thừa. Phải viết lại toàn bộ bằng tổ hợp. Bài học: kế thừa mô tả **loại** (taxonomy), không phải cách tái sử dụng code.

## Kiến thức

### 1. Câu hỏi kiểm tra: "X là một loại Y" hay "X có một Y"
- **Là một loại (is-a):** `Led` là một loại `OutputDevice`. `DerivedException` là một loại `Exception`. Quan hệ này hợp lý cho kế thừa: `Led` có thể dùng **ở bất cứ đâu** cần một `OutputDevice`, và điều đó đúng về mặt ý nghĩa (nguyên tắc thay thế Liskov).
- **Có một (has-a):** `TempSensor` **có một** `I2cBus` để giao tiếp; nó không "là" cái bus. Quan hệ này nên biểu diễn bằng **tổ hợp**: một thành viên hoặc tham chiếu tới đối tượng kia.

Dấu hiệu cảnh báo kế thừa sai chỗ: lớp con **ghi đè** hầu hết hàm của lớp cha để "vô hiệu hoá" hành vi không mong muốn, hoặc lớp con không thật sự thay thế được cho lớp cha ở mọi nơi.

### 2. Ví dụ sai và đúng trên cùng bài toán

{{code:chapter-13/composition_vs_inheritance.cpp}}

{{out:chapter-13/composition_vs_inheritance}}

`TempSensorBad : public I2cBus` biên dịch được và chạy được — đó chính là điều nguy hiểm: **không có lỗi cú pháp nào báo cho bạn biết thiết kế sai**. Vấn đề chỉ lộ ra khi yêu cầu thay đổi (hai cảm biến chung bus). `TempSensor` (đúng) giữ một `I2cBus&`: không sở hữu bus, chỉ tham chiếu tới nó; hai đối tượng `TempSensor` khác nhau có thể cùng tham chiếu **một** `bus`, đúng như phần cứng thật.

### 3. Kế thừa dùng đúng chỗ: giao diện thiết bị đầu ra

{{code:chapter-13/actuator_hierarchy.cpp}}

{{out:chapter-13/actuator_hierarchy}}

Ở đây kế thừa hợp lý vì: `Led` và `Buzzer` **thực sự** đều là "một loại thiết bị output" theo đúng nghĩa dùng trong chương trình — cả hai có khái niệm bật/tắt, và hàm `alarm_sequence()` dùng được với **bất kỳ** `OutputDevice` nào, không cần biết cụ thể là gì. Đây là điểm khác biệt với ví dụ sai ở trên: `TempSensorBad` không thể dùng ở nơi cần một `I2cBus` theo cách có ý nghĩa (không ai muốn "đọc thanh ghi của một cảm biến" như thể nó là cả cái bus).

Chú ý `virtual ~OutputDevice() = default;` — bắt buộc phải có khi lớp có hàm ảo và có thể bị xoá qua con trỏ lớp cơ sở; chương 14 giải thích chi tiết hậu quả nếu thiếu.

### 4. Kế thừa riêng tư và bảo vệ (nhắc nhanh)
`class D : private B` hoặc `protected B` tồn tại trong C++ nhưng **hiếm khi cần** trong firmware. Nếu bạn thấy mình muốn kế thừa riêng tư ("dùng lại code của B nhưng không muốn D là một B"), đó thường là dấu hiệu nên dùng **tổ hợp** thay vì kế thừa dưới bất kỳ hình thức nào.

## Ví dụ thực tiễn: khi nào một driver "nên" kế thừa
Bảng quyết định nhanh cho firmware:

| Tình huống | Nên dùng |
|------------|----------|
| Nhiều loại cảm biến, mã ứng dụng xử lý qua một giao diện chung (`read()`) | Kế thừa từ lớp trừu tượng (interface) — chương 14 |
| Một driver cần dùng một bus/ngoại vi dùng chung với driver khác | Tổ hợp (giữ tham chiếu/con trỏ tới bus) |
| Muốn tái sử dụng vài hàm tiện ích (ví dụ tính CRC) giữa nhiều lớp không liên quan | Hàm tự do (free function) hoặc lớp tiện ích riêng, không kế thừa |
| Nhiều biến thể phần cứng của **cùng một khái niệm** (LED thường, LED RGB, LED qua PWM) | Kế thừa từ `OutputDevice`, hoặc template nếu chọn được lúc biên dịch (chương 15) |

## Góc nhúng
- Tổ hợp bằng tham chiếu (`I2cBus&`) không tốn thêm bộ nhớ ngoài kích thước một con trỏ; không có vtable liên quan đến quan hệ has-a này.
- Kế thừa từ lớp có hàm ảo thêm đúng một vptr cho toàn bộ cây kế thừa (không cộng dồn theo số tầng nếu không có đa kế thừa phức tạp) — xem lại chương 3.
- Đa kế thừa (`class D : public B1, public B2`) tồn tại nhưng hiếm dùng trong firmware; nó làm layout đối tượng và độ phân giải tên phức tạp hơn đáng kể. Ưu tiên tổ hợp hoặc một giao diện thuần ảo duy nhất.

## Lỗi thường gặp
- Kế thừa để "tiết kiệm gõ phím" thay vì vì quan hệ is-a thật sự.
- Không thể trả lời được câu "một `Derived*` có dùng thay cho `Base*` ở MỌI nơi mà vẫn đúng nghĩa không?" — nếu không, đó là dấu hiệu sai chỗ.
- Giữ tham chiếu (`I2cBus&`) tới một đối tượng có vòng đời **ngắn hơn** đối tượng giữ nó (dangling reference) — người tạo `TempSensor` phải đảm bảo `bus` sống lâu hơn `temp`.
- Đa kế thừa nhiều lớp cơ sở không liên quan "cho tiện", dẫn tới nhập nhằng tên hàm khó gỡ.

## Bài tập
1. Thêm một lớp `PressureSensor` dùng chung `I2cBus` với `TempSensor` trong `main()`; chứng minh cả ba cảm biến có thể tồn tại đồng thời trên cùng một `I2cBus bus(1);`.
2. Viết một phản ví dụ: một lớp `Rectangle` và lớp con `Square : public Rectangle` ghi đè `set_width()` để luôn đặt cả `width` và `height` bằng nhau. Giải thích bằng ví dụ cụ thể vì sao điều này vi phạm nguyên tắc thay thế Liskov (gợi ý: viết một hàm nhận `Rectangle&`, đặt `width=5, height=10`, rồi kiểm tra diện tích).
3. Trong `actuator_hierarchy.cpp`, thêm `class RgbLed : public OutputDevice` với ba giá trị màu; `on()`/`off()` có ý nghĩa gì với RGB LED? Nếu không rõ ràng, đó có phải dấu hiệu cần thiết kế lại giao diện `OutputDevice` không?
4. Giải thích bằng lời (không cần code): vì sao `TempSensor` giữ `I2cBus&` (tham chiếu) thay vì `I2cBus` (giữ hẳn một bản sao) là lựa chọn đúng ở đây.

## Tóm tắt
- Kế thừa mô tả "là một loại của" (is-a); tổ hợp mô tả "có một" (has-a). Nhầm hai quan hệ này là nguồn thiết kế cứng nhắc phổ biến nhất.
- Firmware nên **ưu tiên tổ hợp**: driver giữ tham chiếu/con trỏ tới bus hoặc ngoại vi dùng chung.
- Kế thừa hợp lý khi nhiều lớp con thật sự thay thế được cho nhau qua cùng một giao diện.
- Đa kế thừa và kế thừa riêng tư tồn tại nhưng hiếm cần trong firmware; nghi ngờ trước khi dùng.
