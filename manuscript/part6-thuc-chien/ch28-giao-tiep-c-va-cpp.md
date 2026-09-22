---
chapter: 28
title: "Giao tiếp C và C++: extern \"C\", name mangling, liên kết"
part: 6
code: code/chapter-28
---

# Chương 28. Giao tiếp C và C++

## Mục tiêu
- Hiểu name mangling là gì, và vì sao `extern "C"` cần thiết để C++ nói chuyện được với code C (vendor SDK, thư viện cũ, ISR — chương 24).
- Bọc một API kiểu C (struct dữ liệu thô, callback qua con trỏ hàm + `void* ctx`) bằng một lớp C++ RAII.
- Biết giới hạn: kiểu gì được phép qua ranh giới C/C++, và "trampoline" là gì.

## Câu chuyện: linker báo "undefined reference" dù hàm rõ ràng đã định nghĩa
Một đội tích hợp thư viện driver cảm biến do nhà sản xuất cung cấp dưới dạng file `.a` biên dịch bằng C. Header khai báo `void sensor_init(SensorConfig* cfg);`. Gọi từ C++ báo lỗi linker: `undefined reference to 'sensor_init(SensorConfig*)'` — chú ý cặp ngoặc đơn trong thông báo lỗi. Nguyên nhân: trình biên dịch C++ đã **đổi tên** hàm khi tìm kiếm ký hiệu (name mangling), trong khi thư viện `.a` được biên dịch bằng C giữ nguyên tên `sensor_init`. Thiếu `extern "C"` trong khai báo phía C++ là nguyên nhân kinh điển của lỗi này.

## Kiến thức

### 1. Name mangling và `extern "C"`
C++ hỗ trợ nạp chồng hàm (`describe(int)` và `describe(double)` là hai hàm khác nhau) — để làm được điều này, trình biên dịch **mã hoá kiểu tham số vào tên ký hiệu** lúc biên dịch (ví dụ đại khái `_Z8describei` cho `describe(int)`, tuỳ ABI). C không có nạp chồng, nên trình biên dịch C giữ nguyên tên hàm y hệt mã nguồn.

`extern "C"` báo cho trình biên dịch C++: "dùng quy tắc đặt tên kiểu C cho khai báo này" — nhờ đó tên ký hiệu khớp với những gì thư viện C mong đợi (hoặc khớp với bảng vector ngắt của startup file, như chương 24).

{{code:chapter-28/c_interop.cpp}}

{{out:chapter-28/c_interop}}

Phần 3 của ví dụ minh hoạ trực tiếp: `describe(int)`/`describe(double)` nạp chồng bình thường vì có C++ linkage; nếu khai báo chúng là `extern "C"`, chương trình **không biên dịch được** — hai hàm cùng tên C linkage là xung đột định nghĩa, vì tên ký hiệu C không còn chỗ để mã hoá kiểu tham số.

### 2. Bọc API kiểu C bằng RAII (C++ gọi C)
Phần 1–2 của ví dụ mô phỏng một thư viện vendor viết bằng C: `CSensorConfig` là **POD** (Plain Old Data — chỉ có dữ liệu, không constructor/destructor/hàm ảo) nên có bố cục bộ nhớ giống hệt nhau dù được biên dịch bởi trình dịch C hay C++ (miễn cùng ABI của nền tảng — kiểm tra khi trộn hai trình biên dịch khác hãng). `SensorHandle` bọc API đó thành một lớp C++ với constructor gọi `c_sensor_init`, biến `struct CSensorConfig` cục bộ thay vì buộc người dùng tự quản lý — đúng tinh thần RAII của chương 12, áp lên một API vốn không có khái niệm đối tượng.

### 3. Callback qua ranh giới: "trampoline"
API C chỉ hiểu con trỏ hàm thường (`void(*)(void*, int16_t)`) — không có khái niệm phương thức gắn với đối tượng, không có lambda có capture (chương 16), không có `std::function`. Để một đối tượng C++ (`SensorHandle`) nhận callback và cập nhật **trạng thái của chính nó**, cần một hàm trung gian gọi là **trampoline**:

```cpp
static void trampoline(void* ctx, std::int16_t value) {
    static_cast<SensorHandle*>(ctx)->last_value_ = value;
}
```

`trampoline` là hàm **static** (không phải phương thức thường — phương thức thường có tham số `this` ẩn, không khớp chữ ký con trỏ hàm C), nhận `ctx` (chính là `this` được truyền qua `void*` lúc đăng ký callback), ép kiểu lại rồi gọi đúng đối tượng. Đây là mẫu bạn sẽ gặp lại ở hầu hết mọi API C nhận callback: HAL vendor, thư viện C, thậm chí một số API hệ điều hành.

**Lưu ý về tính di động:** gán một hàm `static` (C++ linkage) cho con trỏ kiểu C linkage như trên **không được chuẩn C++ đảm bảo tuyệt đối** (về lý thuyết, hai linkage có thể dùng calling convention khác nhau) — nhưng trên thực tế, mọi trình biên dịch chính thống (GCC, Clang, MSVC, các trình biên dịch cho ARM) đều dùng chung một calling convention cho cả C và C++ trên cùng nền tảng, nên mẫu trampoline này **cực kỳ phổ biến** trong thực tế (chính các SDK vendor, Qt, GTK... dùng) và an toàn trong thực hành, dù không phải "bảo đảm theo chữ" của chuẩn.

## Góc nhúng
- ISR (chương 24) là một trường hợp đặc biệt của `extern "C"`: bảng vector ngắt do startup file (thường viết bằng C/assembly) tạo ra, nên tên hàm ISR phải giữ nguyên.
- Khi trộn `.c` và `.cpp` trong cùng dự án CMake, đảm bảo mọi header dùng chung giữa hai phía có bọc `#ifdef __cplusplus extern "C" { #endif ... }` — mẫu này rất phổ biến trong header vendor, giúp cùng một file `.h` dùng được từ cả C lẫn C++.
- Không truyền đối tượng C++ có constructor/destructor không tầm thường (`std::string`, `std::vector`...) qua ranh giới C — phía C không biết gọi constructor/destructor, dữ liệu sẽ bị hiểu sai hoặc rò tài nguyên.

## Lỗi thường gặp
- Quên `extern "C"` khi khai báo hàm C++ dùng làm callback cho thư viện C hoặc ISR — lỗi linker khó hiểu (`undefined reference` với tên đã bị mangle).
- Cố dùng phương thức thường (non-static) làm con trỏ hàm C — không biên dịch được vì thiếu tham số `this` ẩn không khớp chữ ký mong đợi.
- Dùng lambda **có capture** làm callback kiểu C — chỉ lambda không capture mới chuyển được sang con trỏ hàm thường (nhắc lại chương 16); lambda có capture cần đi qua `ctx` như ví dụ.
- Định nghĩa struct dùng chung C/C++ nhưng sau đó thêm constructor hay hàm ảo vào phía C++ — phá vỡ tính tương thích POD, phía C không còn hiểu đúng bố cục bộ nhớ.
- Trộn cấp phát: `malloc` bên C, `delete` bên C++ (hoặc ngược lại) trên cùng một vùng nhớ — hai bộ cấp phát khác nhau, hành vi không xác định.

## Bài tập
1. Xoá `extern "C"` trước `c_sensor_init` (phần định nghĩa) trong ví dụ và thử tưởng tượng (không cần chạy) linker sẽ báo lỗi gì nếu khai báo trong khối `extern "C" { ... }` và định nghĩa lại khớp nhau hay lệch nhau.
2. Thêm một callback thứ hai `CErrorCallback` (báo lỗi cảm biến) vào `SensorHandle`, viết trampoline tương ứng.
3. Thử biến `trampoline` thành phương thức **không** static và quan sát thông báo lỗi biên dịch khi gán nó cho `CReadyCallback`.
4. Viết một header giả lập kiểu vendor: bọc khai báo struct và hàm trong `#ifdef __cplusplus extern "C" { #endif ... #ifdef __cplusplus } #endif`, giải thích tác dụng của mẫu này khi cùng một `.h` được `#include` bởi cả file `.c` và `.cpp` trong dự án.

## Tóm tắt
- C++ đổi tên hàm để hỗ trợ nạp chồng (name mangling); `extern "C"` tắt việc đó để khớp với ký hiệu mà code C/linker mong đợi.
- Bọc API kiểu C bằng lớp C++ RAII giữ được sự tiện lợi của C++ mà không đổi ABI của thư viện gốc.
- Callback kiểu C cần một hàm trampoline `static` làm cầu nối tới đối tượng C++ thật, truyền qua `void* ctx`.
