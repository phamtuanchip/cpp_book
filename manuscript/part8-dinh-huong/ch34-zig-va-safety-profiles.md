---
chapter: 34
title: "Zig và các đề xuất Safety Profiles cho C++"
part: 8
code: code/chapter-34
---

# Chương 34. Zig và Safety Profiles cho C++

## Mục tiêu
- Nắm triết lý cốt lõi của Zig (đơn giản, không ẩn control flow, `comptime`, allocator tường minh) qua so sánh với các khái niệm C++ đã học.
- Biết C++ Core Guidelines (Bounds/Type/Lifetime safety) và các đề xuất "safety profiles" đang được thảo luận cho các chuẩn C++ tương lai nhắm giải quyết vấn đề gì.
- Tự viết một wrapper kiểm tra biên và một kiểu "không bao giờ null" để cảm nhận tinh thần đó ngay trong C++ hiện tại.

> **Lưu ý về tính thời sự:** các đề xuất "safety profiles"/"Safe C++" cho chuẩn C++ đang được Uỷ ban tiêu chuẩn (WG21) thảo luận tích cực và **thay đổi liên tục** giữa các kỳ họp. Nội dung chương này mô tả **hướng đi chung**, không phải đặc tả cuối cùng — tra các bài báo WG21 mới nhất nếu cần thông tin chính xác về tình trạng chuẩn hoá.

## Câu chuyện: hai triết lý cho cùng một nỗi lo
C++ giải quyết an toàn bằng cách **thêm dần** công cụ lên trên một ngôn ngữ đã tồn tại nửa thế kỷ: RAII, smart pointer, `span`, `optional`, phân tích tĩnh, safety profiles — mỗi lớp giảm một phần rủi ro, không lớp nào bắt buộc. Zig chọn cách khác: một ngôn ngữ **mới, nhỏ**, thiết kế lại từ đầu để không có gì "ẩn" — không nạp chồng toán tử ẩn chi phí, không exception ẩn luồng điều khiển, không cấp phát ẩn sau lưng bạn. Cả hai đều nhắm cùng một nỗi lo của lập trình nhúng: **biết chính xác chương trình sẽ làm gì và tốn bao nhiêu**, chỉ khác con đường tới đó.

## Kiến thức

### 1. Zig: allocator tường minh, `comptime`, error union
Ba ý tưởng của Zig có tương đồng trực tiếp với các chương trước:

```zig
// Minh hoa cu phap Zig (KHONG phai vi du chay duoc cua sach - moi truong bien dich khong co Zig).
const std = @import("std");

fn readSensor(allocator: std.mem.Allocator, bus: *I2cBus, addr: u8) !u16 {
    const buf = try allocator.alloc(u8, 2);  // allocator LUON tuong minh: khong co "heap an"
    defer allocator.free(buf);
    return try bus.readReg(addr, 0x00);      // '!' bao ham loi: goi phai xu ly hoac "try" day len
}
```

- **Allocator tường minh**: mọi hàm cần cấp phát bộ nhớ **nhận allocator làm tham số** — không có heap toàn cục ẩn. Đây chính xác là ý tưởng đằng sau `std::pmr` (chương 22): C++ thêm nó như một lựa chọn *có thể bật*, Zig biến nó thành **cách duy nhất** để cấp phát.
- **`comptime`**: chạy code lúc biên dịch, tổng quát hơn `constexpr`/template của C++ (chương 15, 19) — cùng một cú pháp cho cả "hàm thường" và "hàm chạy lúc biên dịch", không cần từ khoá riêng hay hai cách viết khác nhau.
- **Error union (`!T`)**: kiểu trả về vừa mang giá trị vừa mang lỗi, **bắt buộc** người gọi xử lý (`try` đẩy lỗi lên, hoặc `catch` bắt tại chỗ) — cùng tinh thần với `Result<T>`/`std::expected` ở chương 19, nhưng là cú pháp cấp một của ngôn ngữ, không phải thư viện tự viết.

Zig **không có** exception, không có nạp chồng toán tử, không có RAII/destructor tự động theo nghĩa C++ (dọn dẹp thường tường minh qua `defer`) — lược bỏ những gì Zig coi là "kiểm soát ẩn", đổi lấy việc phải viết tường minh hơn ở một số chỗ.

### 2. C++ Core Guidelines và "safety profiles"
[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/) (Bjarne Stroustrup, Herb Sutter và cộng đồng) chia rủi ro C++ thành các nhóm gọi là **profile**, ba nhóm được nhắc tới nhiều nhất:
- **Bounds safety**: không có truy cập mảng/con trỏ vượt giới hạn không được kiểm tra.
- **Type safety**: không có ép kiểu ngầm định nguy hiểm (`reinterpret_cast` bừa bãi, `union` không kiểm soát).
- **Lifetime safety**: không có con trỏ/tham chiếu dùng sau khi đối tượng bị huỷ (đúng lớp bug ở chương 33).

Đây là **hướng đi**, không phải tính năng có sẵn trong C++20/23 mà sách dùng — công cụ hỗ trợ hiện tại là **thư viện** (GSL — Guidelines Support Library — cung cấp `gsl::not_null`, `gsl::span` với kiểm tra biên) và **phân tích tĩnh** (chương 30, clang-tidy có bộ kiểm tra riêng cho Core Guidelines). Các đề xuất mới hơn đang thảo luận việc đưa một số kiểm tra này **vào chuẩn ngôn ngữ**, gần với cách Rust bắt lỗi lúc biên dịch (chương 33) hơn — nhưng cần tương thích ngược với hàng tỷ dòng C++ đã tồn tại, nên tiến trình chậm và thận trọng.

### 3. Tự viết wrapper theo tinh thần safety profile

{{code:chapter-34/safety_profile_style.cpp}}

{{out:chapter-34/safety_profile_style}}

- `CheckedSpan<T>::at` kiểm tra chỉ số **mỗi lần truy cập** bằng `assert` — đúng tinh thần Bounds safety: một truy cập sai chỉ số **dừng chương trình ngay tại chỗ sai** (dễ chẩn đoán), thay vì đọc/ghi âm thầm vào vùng nhớ không thuộc về nó (hành vi không xác định, có thể không lộ ra cho tới rất lâu sau).
- `NotNull<T>` kiểm tra **một lần** lúc khởi tạo — mọi nơi nhận `NotNull<T>` (như `print_value`) được đảm bảo (bởi kiểu, không phải bởi kỷ luật nhớ kiểm tra) rằng con trỏ không null, loại bỏ hẳn nhu cầu viết `if (ptr)` lặp lại khắp nơi.
- Cả hai đều dùng `assert` — trong build release (`-DNDEBUG`) `assert` bị **tắt hoàn toàn**, mất luôn kiểm tra. Với code cần kiểm tra cả ở release, cần cơ chế riêng (kiểm tra tường minh + xử lý lỗi như `Result<T>` chương 19, không dựa vào `assert`).

GSL thật (`gsl::not_null`, `gsl::span`) cung cấp các kiểu này đầy đủ hơn — tự viết ở đây để hiểu **cơ chế**, giống tinh thần ETL ở chương 32.

## Góc nhúng
- `assert` gây chi phí runtime (một phép so sánh + có thể gọi `abort`); trong code chạy trong vòng lặp nóng, cân nhắc build riêng có/không `assert` để đo tác động, không tắt `assert` "cho chắc" ở mọi nơi.
- `comptime` của Zig và `constexpr`/template của C++ giải quyết cùng lớp bài toán (tính toán lúc biên dịch, chuyên biệt hoá theo kiểu) nhưng với độ phức tạp cú pháp khác nhau — một tiêu chí đáng cân nhắc khi đánh giá ngôn ngữ cho dự án mới.
- Toolchain Zig cho nhúng (ARM, RISC-V) còn non trẻ hơn GCC/Clang cho C++ — kiểm tra hỗ trợ MCU cụ thể trước khi đặt cược một dự án sản xuất vào đó.

## Lỗi thường gặp
- Dùng `assert` cho kiểm tra **phải luôn đúng ở production** (ví dụ xác thực dữ liệu từ bên ngoài) — `assert` biến mất ở build release, để lỗ hổng lọt qua.
- Tưởng thêm `NotNull`/`CheckedSpan` là "đã an toàn như Rust" — đây vẫn là kiểm tra **lúc chạy**, không phải **lúc biên dịch**; chi phí và độ trễ phát hiện lỗi khác hẳn borrow checker (chương 33).
- Nhầm `comptime` của Zig với đơn giản là "nhanh hơn" — nó là công cụ metaprogramming, có đường cong học tương tự template C++, không phải phép màu miễn phí.
- Theo dõi các đề xuất "safety profiles" như thể chúng đã là một phần chuẩn C++ hiện hành — kiểm tra bài báo WG21 và trạng thái chuẩn hoá thực tế trước khi lập kế hoạch dự án dựa vào chúng.

## Bài tập
1. Thêm phương thức `front()`/`back()` có kiểm tra biên vào `CheckedSpan`.
2. Viết một `NotNull<T>` có thể **gán lại** con trỏ khác (hiện tại chỉ gán lúc tạo) mà vẫn giữ bất biến "không bao giờ null" — kiểm tra ở cả constructor lẫn hàm gán.
3. Đọc phần "Bounds safety profile" trong tài liệu C++ Core Guidelines chính thức; so sánh với `CheckedSpan` tự viết — nó còn thiếu gì so với mô tả đầy đủ?
4. Nếu tò mò, thử biên dịch một chương trình Zig "Hello World" nhỏ (cài Zig từ trang chính thức) và so sánh trải nghiệm build (không cần Makefile/CMake) với `npm run verify` của sách.

## Tóm tắt
- Zig theo đuổi an toàn/dễ đoán bằng cách loại bỏ điều "ẩn" (allocator tường minh, không exception, `comptime` thống nhất) trong một ngôn ngữ mới, nhỏ.
- C++ theo đuổi cùng mục tiêu bằng cách thêm dần công cụ (GSL, phân tích tĩnh, safety profiles đang đề xuất) lên trên ngôn ngữ đã có, ưu tiên tương thích ngược.
- Tự viết wrapper kiểm tra biên/null (`CheckedSpan`, `NotNull`) giúp cảm nhận tinh thần "safety profile" ngay trong C++ hiện tại, dù đó vẫn là kiểm tra lúc chạy, không phải bảo đảm lúc biên dịch như Rust.
