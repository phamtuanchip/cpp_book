---
chapter: 31
title: "Tối ưu kích thước code và tốc độ"
part: 7
code: code/chapter-31
---

# Chương 31. Tối ưu kích thước code và tốc độ

## Mục tiêu
- Biết các cờ trình biên dịch/linker ảnh hưởng tới kích thước Flash: `-Os`/`-O2`, LTO, `--gc-sections`.
- Nhận diện và sửa "phình Flash do template" (template bloat) bằng cách tách phần logic không phụ thuộc tham số template.
- Giữ nguyên tắc xuyên suốt cuốn sách: **đo trước khi tối ưu**, không đoán.

## Câu chuyện: thêm một tính năng nhỏ, Flash tăng vọt
Một driver cảm biến hỗ trợ nhiều kiểu số (`int16_t`, `int32_t`, `float`) qua template. Sau khi thêm hỗ trợ mảng kích thước tuỳ chỉnh (`std::array<T, N>` với nhiều `N` khác nhau ở các nơi gọi khác nhau trong dự án), bản build tăng vài KB Flash dù không thêm tính năng "thật" nào — mỗi tổ hợp `(T, N)` sinh một bản mã máy riêng, phần lớn giống hệt nhau ngoại trừ một số N vòng lặp.

## Kiến thức

### 1. `-Os` so với `-O2`, và LTO
- `-O2`: ưu tiên **tốc độ**, chấp nhận sinh mã lớn hơn (inline nhiều hơn, mở vòng lặp — loop unrolling).
- `-Os`: ưu tiên **kích thước**, tắt bớt các tối ưu làm tăng code (vẫn giữ hầu hết tối ưu không đổi kích thước). Lựa chọn phổ biến cho MCU Flash nhỏ.
- **LTO (Link-Time Optimization, `-flto`)**: cho phép trình biên dịch tối ưu **xuyên file nguồn** thay vì mỗi file riêng lẻ — loại bỏ hàm không dùng tới xuyên toàn dự án, inline qua ranh giới file. Có thể giảm đáng kể kích thước lẫn tăng tốc độ, đổi lại thời gian build lâu hơn.

Không có cờ nào "luôn đúng" — đo cả hai trên chính firmware của bạn (`arm-none-eabi-size`, chương 20) rồi quyết định.

### 2. `--gc-sections`: loại bỏ code không dùng tới
Biên dịch với `-ffunction-sections -fdata-sections` đặt **mỗi hàm/biến vào một section riêng** thay vì gộp chung; link với `-Wl,--gc-sections` để linker loại bỏ các section không có gì tham chiếu tới. Hữu ích đặc biệt khi:
- Dùng một thư viện lớn (SDK vendor, thư viện chuẩn) nhưng chỉ gọi vài hàm — phần còn lại bị loại nếu không ai gọi tới (kể cả gián tiếp).
- Có nhiều bản instantiation của template (mục 3) không thực sự được dùng ở một số cấu hình build.

Không có tác dụng với hàm **có thể** được gọi (ví dụ qua con trỏ hàm lưu trong bảng, hoặc `extern "C"` xuất ra ngoài) — linker phải giữ lại vì "có thể ai đó cần".

### 3. Phình Flash do template và cách giảm

{{code:chapter-31/template_bloat.cpp}}

{{out:chapter-31/template_bloat}}

`sum_bloat<int, N>` với ba giá trị `N` khác nhau (4, 10, 3) sinh ra **ba thân hàm riêng biệt** — dù logic hoàn toàn giống nhau, chỉ khác số vòng lặp. Đây là hệ quả trực tiếp của cách template hoạt động (chương 15): mỗi tổ hợp tham số là một kiểu/hàm instantiation riêng.

`sum_shared` tách phần logic thành `sum_shared_impl<T>` nhận `(con trỏ, độ dài)` thay vì `std::array<T, N>` — giờ chỉ còn **một bản mã theo mỗi kiểu `T`** (ở đây là `int`), dùng chung cho mọi `N`. Phần template còn lại (`sum_shared`) chỉ còn một dòng chuyển đổi `array` → `(con trỏ, độ dài)`, gần như không tốn thêm Flash khi có `N` mới.

Nguyên tắc tổng quát: **tách phần logic không thực sự cần biết giá trị tham số template ra khỏi template**, để nó được biên dịch một lần và dùng chung. Đây cũng chính xác là cách nhiều triển khai `std::vector` nội bộ tổ chức code (phần quản lý bộ nhớ thô không phụ thuộc kiểu phần tử được tách riêng).

### 4. Các nguồn phình Flash khác cần đo trước khi kết luận
- **`<iostream>`**: kéo theo bộ máy khởi tạo stream khá lớn ngay cả khi chỉ dùng `std::cout << "x"`; `<cstdio>` (như toàn bộ ví dụ của sách) thường nhẹ hơn đáng kể trên toolchain nhúng — nhưng **đo trên toolchain cụ thể của bạn**, chênh lệch phụ thuộc thư viện chuẩn (newlib, newlib-nano, picolibc...).
- **Exception/RTTI** (`-fno-exceptions -fno-rtti`): loại bỏ bảng unwind và thông tin kiểu runtime nếu không dùng `throw`/`dynamic_cast`/`typeid` — tiết kiệm đáng kể trên một số toolchain, cũng cần đo cụ thể.
- **`printf` với hỗ trợ số thực đầy đủ**: một số toolchain nhúng có bản `printf` rút gọn (không hỗ trợ `%f`) nhẹ hơn nhiều so với bản đầy đủ — kiểm tra tài liệu toolchain/newlib-nano nếu Flash eo hẹp.

## Góc nhúng
- Luôn đo bằng `arm-none-eabi-size`/file `.map` (chương 20) **trước và sau** mỗi thay đổi tối ưu — cảm giác "chắc sẽ nhỏ hơn" thường sai với trình biên dịch tối ưu hiện đại.
- Compiler Explorer cho phép thử nhanh `-Os`/`-O2`/`-flto` và xem assembly/kích thước ước lượng mà không cần setup toolchain đầy đủ.
- Tối ưu tốc độ (vòng lặp nóng, ISR) và tối ưu kích thước (code ít chạy) thường **mâu thuẫn nhau** — có thể cần `-Os` toàn cục nhưng đánh dấu riêng vài hàm nóng bằng `__attribute__((optimize("O2")))` (đặc thù GCC, kiểm tra cú pháp đúng phiên bản) hoặc tách file biên dịch cờ khác nhau.

## Lỗi thường gặp
- Tối ưu "theo cảm giác" mà không đo — nhiều thay đổi tưởng làm nhỏ code lại làm to hơn (hoặc ngược lại) trên trình biên dịch hiện đại.
- Dùng template với nhiều tham số kích thước (`N`) khác nhau cho logic giống hệt nhau mà không tách phần dùng chung.
- Bật `-Os` toàn cục rồi ngạc nhiên khi vòng lặp xử lý tín hiệu thời gian thực chậm đi — cần đo tốc độ, không chỉ kích thước.
- Quên `-ffunction-sections`/`--gc-sections` khi liên kết với thư viện lớn, giữ lại code không bao giờ được gọi.

## Bài tập
1. Thêm một instantiation thứ tư của `sum_bloat` với `N = 20`; giải thích (không cần đo thật nếu không có toolchain) code sẽ tăng thêm một bản nữa.
2. Viết `sum_shared_impl` tổng quát cho phép truyền thêm một hàm so sánh tuỳ biến (giống `std::max_element`) mà vẫn giữ được lợi ích dùng chung theo `N`.
3. Tra cứu (Compiler Explorer hoặc tài liệu GCC/Clang) sự khác biệt cụ thể giữa `-O2` và `-Os` về loop unrolling; ghi lại ít nhất hai tối ưu bị tắt ở `-Os`.
4. Nếu có toolchain `arm-none-eabi-gcc`, biên dịch cùng một chương trình với `-O2` và `-Os`, so sánh cột `text` bằng `arm-none-eabi-size`.

## Tóm tắt
- `-Os`/`-O2`/LTO/`--gc-sections` là các công cụ ở tầng build, không phải code, để kiểm soát kích thước/tốc độ — không có lựa chọn đúng tuyệt đối, phải đo.
- Phình Flash do template thường đến từ việc để logic không phụ thuộc tham số template nằm bên trong template — tách nó ra một hàm dùng chung.
- Nguyên tắc xuyên suốt cuốn sách áp dụng lại ở đây: đo bằng công cụ thật (`size`, `.map`, Compiler Explorer) trước khi tin vào trực giác về tối ưu.
