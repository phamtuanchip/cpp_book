---
chapter: 14
title: "Đa hình động và thiết kế giao diện driver"
part: 3
code: code/chapter-14
---

# Chương 14. Đa hình động và thiết kế giao diện driver

## Mục tiêu
- Thiết kế một giao diện thuần ảo (`ISensor`) và trình quản lý xử lý nhiều loại cảm biến qua cùng một giao diện.
- Hiểu chính xác vì sao thiếu `virtual` ở destructor gây lỗi, và cách trình biên dịch có thể cảnh báo.
- Biết khi nào đa hình động (chương này) là lựa chọn đúng, khi nào nên dùng template (chương 15).

## Câu chuyện: thêm cảm biến mới không được sửa code cũ
Firmware gốc chỉ có một loại cảm biến nhiệt độ, code đọc thẳng `read_temp_raw()`. Sáu tháng sau, sản phẩm thêm biến thể dùng cảm biến khác (chip khác, giao thức khác). Cách tệ: thêm `#ifdef SENSOR_V2` rải khắp nơi. Cách tốt: định nghĩa một giao diện chung (`ISensor`) từ đầu; thêm cảm biến mới là thêm **một lớp mới cài đặt giao diện**, không sửa code đã có — đúng nguyên lý "mở để mở rộng, đóng để sửa đổi" (nguyên tắc O trong SOLID).

## Kiến thức

### 1. Lớp trừu tượng thuần ảo làm hợp đồng
`ISensor` bên dưới không có dữ liệu, chỉ có các hàm thuần ảo (`= 0`): nó là một **hợp đồng** — bất kỳ ai cài đặt các hàm đó đều được mã ứng dụng chấp nhận, không cần biết chi tiết bên trong.

{{code:chapter-14/sensor_manager.cpp}}

{{out:chapter-14/sensor_manager}}

Các điểm thiết kế:
- `virtual ~ISensor() = default;` — bắt buộc (xem mục 3).
- `[[nodiscard]] virtual int read() = 0;` — thuần ảo (`= 0`): lớp không cài đặt hàm này **không thể** được tạo đối tượng (compiler báo lỗi ngay), đảm bảo mọi cảm biến thực sự đọc được.
- `class FakeTemp final : public ISensor` — từ khoá `final` báo không ai được kế thừa tiếp từ `FakeTemp`; hữu ích khi bạn chắc chắn đây là điểm dừng của cây kế thừa, và giúp trình biên dịch tối ưu tốt hơn ở một số trường hợp gọi hàm.
- `poll_all()` chỉ phụ thuộc `ISensor`, không biết `FakeTemp`/`FakeHumidity` là gì — thêm loại cảm biến thứ ba **không cần sửa** `poll_all()`.

### 2. Destructor ảo: không phải tuỳ chọn
Quy tắc: **nếu một lớp có bất kỳ hàm ảo nào, hoặc có thể bị xoá qua con trỏ lớp cơ sở, destructor của nó phải là `virtual`.** Không tuân theo quy tắc này là **hành vi không xác định** theo chuẩn C++ — không phải "chậm hơn" mà là "kết quả không được đảm bảo", dù trong thực tế thường biểu hiện thành rò rỉ tài nguyên (phần con không được dọn dẹp).

Cách trình biên dịch giúp bạn: với `-Wall`/`-Wextra` (thực chất là `-Wnon-virtual-dtor`/`-Wdelete-non-virtual-dtor` tuỳ ngữ cảnh), GCC/Clang cảnh báo khi bạn `delete` một đối tượng qua con trỏ tới lớp có hàm ảo nhưng destructor không ảo:

```cpp
struct BaseNoVirtual {
    virtual void ping() { /* ... */ }
    ~BaseNoVirtual() { /* KHÔNG có 'virtual' */ }
};
struct DerivedA : BaseNoVirtual { ~DerivedA() { /* giải phóng tài nguyên ở đây */ } };

BaseNoVirtual* bad = new DerivedA();
delete bad;   // <-- dòng này bị cảnh báo
```

Biên dịch với `g++ -std=c++20 -Wall -Wextra -fsyntax-only`:

```text
warning: deleting object of polymorphic class type 'BaseNoVirtual' which has
non-virtual destructor might cause undefined behavior [-Wdelete-non-virtual-dtor]
```

Bản sửa đúng chỉ cần thêm **một từ khoá**:

{{code:chapter-14/virtual_destructor_ok.cpp}}

{{out:chapter-14/virtual_destructor_ok}}

**Không bao giờ tắt cảnh báo này.** Nếu bạn chắc chắn một lớp sẽ không bao giờ bị kế thừa hoặc bị xoá qua con trỏ cơ sở, đánh dấu nó `final` thay vì bỏ qua cảnh báo.

### 3. Chi phí và giới hạn (nhắc lại, mở rộng)
Chương 3 đã đo: mỗi đối tượng có hàm ảo tốn thêm một con trỏ vptr; mỗi lời gọi hàm ảo là một lần tra bảng rồi nhảy gián tiếp — chậm hơn gọi trực tiếp, và ngăn trình biên dịch inline (vì không biết lúc biên dịch sẽ gọi hàm của lớp con nào). Với `poll_all()` gọi 2 cảm biến vài lần mỗi giây, chi phí này không đáng kể. Với vòng lặp điều khiển động cơ chạy hàng chục nghìn lần mỗi giây, chi phí gọi gián tiếp có thể đáng kể — đó là lúc cân nhắc chương 15 (CRTP/template).

## Ví dụ thực tiễn: trình quản lý cảm biến có thể cấu hình lúc chạy
Tình huống hợp lý cho đa hình động: một sản phẩm có thể lắp **một trong vài loại** cảm biến tuỳ đơn hàng, xác định qua một chân ID đọc lúc khởi động. Chương trình đọc chân ID, `new` (hoặc đặt vào bộ nhớ tĩnh đã cấp phát sẵn — xem chương 20) đối tượng cảm biến tương ứng, rồi từ đó chỉ làm việc qua con trỏ `ISensor*`. Đây là trường hợp **loại đối tượng chỉ biết được lúc chạy** — đúng lý do tồn tại của đa hình động.

## Góc nhúng
- Mỗi lớp cài đặt `ISensor` thêm một vtable **dùng chung cho mọi đối tượng của lớp đó** (không phải mỗi đối tượng); vtable nằm trong Flash (`.rodata`), không tốn RAM.
- Mỗi **đối tượng** (không phải lớp) tốn thêm một con trỏ vptr trong RAM (hoặc bất kỳ nơi đối tượng được đặt).
- Trên MCU rất nhỏ (dưới 1000 đối tượng có hàm ảo là chuyện hiếm gặp), chi phí vtable gần như luôn chấp nhận được; đừng tối ưu sớm khi chưa đo.

## Lỗi thường gặp
- Lớp cơ sở có hàm ảo mà quên `virtual` ở destructor.
- Quên `override` ở hàm ghi đè — nếu chữ ký sai (nhầm kiểu tham số), thiếu `override` khiến trình biên dịch âm thầm coi đó là một hàm **mới**, không ghi đè gì cả; có `override` sẽ báo lỗi ngay khi chữ ký không khớp.
- Gọi hàm ảo từ trong constructor hoặc destructor của lớp cơ sở: tại thời điểm đó, phần "con" của đối tượng chưa được xây dựng (hoặc đã bị huỷ), nên lời gọi luôn phân giải tới phiên bản của lớp cơ sở, không phải lớp con — dễ gây ngạc nhiên.
- Thiết kế giao diện quá lớn (nhiều hàm thuần ảo không phải cảm biến nào cũng cần) — vi phạm nguyên tắc phân tách giao diện (nguyên tắc I trong SOLID); tách thành nhiều giao diện nhỏ nếu cần.

## Bài tập
1. Thêm `class FakePressure final : public ISensor` vào `sensor_manager.cpp` và mở rộng mảng `sensors` lên 3 phần tử **mà không sửa** hàm `poll_all()`.
2. Bỏ `override` khỏi `FakeTemp::read()` và cố tình đổi kiểu trả về thành `long`. Quan sát: chương trình còn biên dịch không? Nó còn ghi đè đúng hàm của `ISensor` không? Thêm lại `override` và xem thông báo lỗi thay đổi thế nào.
3. Giải thích bằng lời vì sao `final` trên `class FakeTemp final : public ISensor` không ảnh hưởng gì đến việc `FakeTemp` cài đặt giao diện `ISensor`, mà chỉ ngăn một lớp thứ ba kế thừa từ `FakeTemp`.
4. Viết một `class NullSensor final : public ISensor` luôn trả về một giá trị cố định và `init()` luôn thành công — dùng làm "cảm biến giả" khi phần cứng chưa lắp, để phần còn lại của firmware không cần kiểm tra con trỏ null ở khắp nơi. Giải thích lợi ích của mẫu thiết kế "Null Object" này.

## Tóm tắt
- Giao diện thuần ảo (`= 0`) định nghĩa hợp đồng chung; nhiều lớp cài đặt cho phép mở rộng mà không sửa code cũ.
- Destructor của lớp cơ sở có hàm ảo **phải** là `virtual`; trình biên dịch cảnh báo khi bạn quên — đừng tắt cảnh báo đó.
- `override` là lưới an toàn miễn phí: bắt lỗi chữ ký sai ngay lúc biên dịch.
- Đa hình động phù hợp khi loại đối tượng cụ thể chỉ xác định được lúc chạy; nếu biết trước lúc biên dịch, cân nhắc template (chương sau).
