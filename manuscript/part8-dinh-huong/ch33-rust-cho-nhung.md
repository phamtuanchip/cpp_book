---
chapter: 33
title: "Rust cho nhúng: so sánh với C++"
part: 8
code: code/chapter-33
---

# Chương 33. Rust cho nhúng: so sánh với C++

## Mục tiêu
- Hiểu **borrow checker** của Rust giải quyết vấn đề gì mà C++ để lại cho kỷ luật lập trình viên.
- Biết Rust nhúng vẫn cần `unsafe` cho MMIO/thanh ghi phần cứng — an toàn bộ nhớ không phải "phép màu" xoá bỏ mọi rủi ro.
- Có tiêu chí thực dụng để chọn giữa Rust và C++ cho một dự án nhúng mới, thay vì tranh luận cảm tính.

## Câu chuyện: một con trỏ "tưởng vẫn còn hợp lệ"
Đoạn code C++ sau biên dịch sạch với `-Wall -Wextra`, không cảnh báo gì:
```cpp
std::vector<int> v{1, 2, 3};
int* p = &v[0];
for (int i = 0; i < 100; ++i) v.push_back(i);  // co the lam vector cap phat lai
std::printf("%d\n", *p);  // p co the da tro vao vung nho da giai phong
```
`push_back` liên tục có thể khiến `std::vector` **cấp phát lại** vùng nhớ lớn hơn khi vượt sức chứa hiện tại — vùng nhớ cũ (nơi `p` đang trỏ tới) bị giải phóng. Đây là **hành vi không xác định**: chương trình có thể chạy "đúng" hàng trăm lần rồi thất bại ngẫu nhiên trên một máy khác hoặc một bản build khác. Rust từ chối biên dịch mã tương đương (mô tả bên dưới), buộc bạn sửa lỗi **trước khi** chương trình từng chạy lần nào.

## Kiến thức

### 1. Borrow checker: kiểm tra vòng đời tham chiếu lúc biên dịch
Rust theo dõi, cho mỗi tham chiếu (`&T`/`&mut T`), nó **mượn** dữ liệu từ đâu và trong bao lâu. Mã Rust minh hoạ cùng bug (chỉ để đọc, **không phải ví dụ chạy được của sách** — môi trường biên dịch của sách không có Rust toolchain):

```rust
let mut v: Vec<i32> = vec![1, 2, 3];
let p: &i32 = &v[0];          // muon bat bien tu v
for i in 0..100 { v.push(i); }  // loi bien dich: push can muon KHA BIEN trong khi p con "song"
println!("{}", p);
```

Trình biên dịch Rust báo lỗi ngay tại dòng `v.push(i)`: không thể mượn `v` khả biến (`push` cần sửa `v`) trong khi `p` (mượn bất biến) vẫn còn được dùng sau đó. Đây chính là quy tắc chương 18 của sách này (RAII/move: một chủ sở hữu, một thời điểm) — Rust **bắt buộc** nó bằng trình biên dịch, C++ chỉ **khuyến nghị** nó bằng kỷ luật lập trình viên và các mẫu thiết kế (move-only, tránh giữ con trỏ/tham chiếu qua thao tác có thể làm invalid).

### 2. Cách viết an toàn tương đương trong C++

{{code:chapter-33/ownership_pitfall.cpp}}

{{out:chapter-33/ownership_pitfall}}

Không có "cú pháp thần kỳ" nào biến C++ thành Rust; hai mẫu an toàn (`safe_pattern_using_index`, `safe_pattern_reference_after_mutation`) đều là **kỷ luật thiết kế**: giữ **chỉ số** (vị trí) thay vì con trỏ/tham chiếu khi container có thể đổi kích thước, hoặc chỉ lấy tham chiếu **sau khi** chắc chắn không còn thao tác nào làm nó invalid. Công cụ hỗ trợ (không thay thế) kỷ luật này: `-fsanitize=address` (ASan) khi test trên host có thể bắt được nhiều lỗi loại này lúc chạy (không phải lúc biên dịch như Rust), và phân tích tĩnh (chương 30) bắt được một phần các mẫu rủi ro.

### 3. Rust nhúng vẫn cần `unsafe`
Truy cập thanh ghi MMIO (chương 21) là thao tác vốn dĩ "không an toàn" theo nghĩa borrow checker không thể suy luận được (giá trị đổi bởi phần cứng, không phải bởi Rust code) — Rust nhúng (crate `cortex-m`, `embedded-hal`...) bọc truy cập thanh ghi trong khối `unsafe { ... }`, nơi lập trình viên **tự chịu trách nhiệm** đúng như viết C++ với `volatile T*`. Borrow checker giúp rất nhiều cho phần **logic ứng dụng** (quản lý bộ nhớ, chia sẻ dữ liệu giữa các phần code), nhưng không xoá bỏ nhu cầu hiểu phần cứng ở lớp driver thấp nhất — hai ngôn ngữ hội tụ về cùng một sự thật vật lý ở lớp đó.

### 4. Chọn Rust hay C++ cho dự án nhúng mới?
| Tiêu chí | Nghiêng về C++ | Nghiêng về Rust |
|---|---|---|
| Vendor SDK/HAL sẵn có cho MCU | C/C++ gần như luôn có; Rust có thể thiếu hoặc chưa trưởng thành cho chip cụ thể | Hệ sinh thái `embedded-hal` đang phát triển nhanh nhưng chưa phủ hết mọi dòng chip |
| Đội ngũ hiện tại | Đã thạo C/C++, code base lớn có sẵn | Sẵn sàng đầu tư học đường cong mới |
| Yêu cầu an toàn bộ nhớ nghiêm ngặt (an toàn tính mạng, bảo mật cao) | Cần kỷ luật + công cụ (ASan, MISRA, chương 30) bù đắp | Borrow checker giảm hẳn một lớp lỗi phổ biến "miễn phí" ở lúc biên dịch |
| Tương thích ngược, tích hợp code C cũ | Trực tiếp, `extern "C"` (chương 28) | Cũng làm được qua FFI, nhưng thêm một lớp chuyển đổi |
| Thời gian biên dịch, công cụ toolchain đã ổn định lâu năm | Có | Đang cải thiện, một số công cụ debug/IDE chưa phong phú bằng |

Không có câu trả lời đúng tuyệt đối — đây là quyết định kỹ thuật **và** tổ chức (nhân sự, thời hạn, hệ sinh thái sẵn có cho phần cứng cụ thể của bạn).

## Góc nhúng
- Cả hai ngôn ngữ đều có thể build `no_std`/không heap cho MCU nhỏ; kích thước Flash cuối cùng phụ thuộc nhiều vào cách viết và cấu hình hơn là bản thân ngôn ngữ.
- Rust và C++ liên kết được với nhau qua FFI/`extern "C"` — một dự án có thể dùng C++ cho phần lớn, Rust cho một module cụ thể (hoặc ngược lại) nếu có lý do rõ ràng (ví dụ một thư viện phân tích giao thức phức tạp, nơi an toàn bộ nhớ đặc biệt quan trọng).
- `-fsanitize=address,undefined` khi chạy test trên host (chương 29) là công cụ C++ gần nhất để bắt các lớp lỗi mà Rust ngăn từ lúc biên dịch — không tương đương hoàn toàn (chạy lúc test, không phải mọi lúc biên dịch) nhưng rất đáng bật trong CI.

## Lỗi thường gặp
- Tin rằng "dùng Rust là tự động an toàn" — `unsafe` vẫn cần và vẫn có thể sai, đặc biệt ở lớp driver phần cứng.
- Tin rằng "C++ hiện đại (RAII, smart pointer) là đủ, không cần công cụ gì thêm" — các mẫu ở chương 33 vẫn có thể bị vi phạm nếu không cẩn thận, khác biệt với Rust là *hệ quả* (runtime) thay vì *lỗi biên dịch*.
- So sánh benchmark hiệu năng hai ngôn ngữ mà không kiểm soát cùng mức tối ưu, cùng thư viện chuẩn, cùng cấu hình — kết luận vội vàng.
- Viết lại toàn bộ code base C++ đang chạy tốt sang Rust chỉ vì xu hướng, không dựa trên phân tích chi phí/lợi ích cụ thể.

## Bài tập
1. Viết một biến thể khác của bug ở mục 1 (ví dụ tham chiếu vào `std::string` bị `resize`), rồi áp dụng cùng nguyên tắc sửa (chỉ số hoặc lấy tham chiếu sau cùng).
2. Nếu có Rust toolchain, cài đặt và thử biên dịch đoạn code Rust ở mục 1 trên [Rust Playground](https://play.rust-lang.org) — đọc chính xác thông báo lỗi của borrow checker.
3. Tìm một crate Rust nhúng (`embedded-hal`, `cortex-m`) trong tài liệu chính thức và đối chiếu API của nó (ví dụ đọc/ghi GPIO) với `GpioPin` ở chương 23 — điểm giống và khác chính là gì?
4. Viết tiêu chí (bảng ở mục 4) áp dụng cho một dự án cụ thể bạn đang làm hoặc dự định làm — kết luận nghiêng về ngôn ngữ nào và vì sao.

## Tóm tắt
- Borrow checker của Rust bắt được ở lúc biên dịch một lớp lỗi (dangling reference, alias không an toàn) mà C++ chỉ ngăn được bằng kỷ luật thiết kế và công cụ hỗ trợ (ASan, phân tích tĩnh).
- Rust nhúng vẫn cần `unsafe` cho MMIO — an toàn bộ nhớ không xoá bỏ nhu cầu hiểu đúng phần cứng ở lớp driver.
- Chọn ngôn ngữ là quyết định thực dụng dựa trên hệ sinh thái phần cứng, đội ngũ, và mức độ nghiêm ngặt về an toàn cần thiết — không phải một câu trả lời đúng cho mọi dự án.
