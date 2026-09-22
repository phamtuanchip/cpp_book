---
chapter: 22
title: "Cấp phát tĩnh, std::pmr và placement new"
part: 5
code: code/chapter-22
---

# Chương 22. Cấp phát tĩnh, `std::pmr` và placement new

## Mục tiêu
- Biết ba cách có đối tượng "động" mà không gọi `malloc`/`new` lên heap hệ thống: pool tĩnh, `std::pmr`, placement new.
- Dùng `std::pmr::monotonic_buffer_resource` để container chuẩn (`vector`, `map`...) cấp phát từ một vùng nhớ tĩnh.
- Hiểu placement new: xây dựng đối tượng vào bộ nhớ có sẵn, và trách nhiệm gọi destructor thủ công đi kèm.

## Câu chuyện: cần `std::vector` nhưng cấm heap
Một firmware cấm `malloc`/heap (yêu cầu an toàn, không phân mảnh — chương 20), nhưng đội phát triển muốn dùng `std::vector` để tận dụng API quen thuộc thay vì viết lại mảng động tay. Giải pháp không phải "viết vector từ đầu", mà là cho `std::vector` một **nơi cấp phát khác** — một vùng nhớ tĩnh cố định — thông qua `std::pmr` (Polymorphic Memory Resource, C++17).

## Kiến thức

### 1. `std::pmr`: đổi "nơi" container cấp phát, không đổi API

{{code:chapter-22/pmr_static_arena.cpp}}

{{out:chapter-22/pmr_static_arena}}

- `std::array<std::byte, 256> arena` là vùng nhớ tĩnh — có thể đặt `static` để nằm ở `.bss` thay vì stack.
- `std::pmr::monotonic_buffer_resource` cấp phát **tuần tự** từ `arena`: nhanh (chỉ dịch con trỏ), không phân mảnh, nhưng **không thu hồi từng phần tử** — chỉ hợp với vòng đời cấp-phát-một-lần-rồi-dùng (ví dụ: dựng một danh sách trong một lần xử lý gói tin, rồi vứt bỏ toàn bộ khi xong).
- Tham số thứ ba, `std::pmr::null_memory_resource()`, là "upstream": nếu `arena` hết chỗ, resource **không** âm thầm rơi về heap hệ thống — nó thất bại ngay (`bad_alloc`). Đây là lựa chọn quan trọng cho nhúng: bạn muốn biết ngay lúc phát triển nếu vùng tĩnh quá nhỏ, không muốn nó "vô tình chạy được" nhờ heap rồi hỏng khi build production tắt heap.
- `std::pmr::vector<int>` giống hệt `std::vector<int>` về API, chỉ khác cách cấp phát bộ nhớ nội bộ.

Container tiêu chuẩn khác (`std::pmr::map`, `std::pmr::string`...) dùng cùng cách này. Với build không có `<memory_resource>` (một số toolchain nhúng cũ hoặc newlib rút gọn), quay lại cách viết tay ở mục 2.

### 2. Pool cố định tự viết (nhắc lại và tổng quát hoá chương 18)
Chương 18 đã viết một `BufferPool` cấp phát ô cố định qua `Handle` move-only. Đó là mẫu tổng quát: **mảng tĩnh + bit/cờ đánh dấu ô đang dùng**, không có `malloc` nào. `std::pmr` (mục 1) chỉ là một lớp vỏ chuẩn hoá quanh cùng ý tưởng, để dùng được với container thư viện chuẩn.

### 3. Placement new: xây dựng đối tượng vào bộ nhớ có sẵn

{{code:chapter-22/placement_new.cpp}}

{{out:chapter-22/placement_new}}

`new (storage) Logger("boot")` gọi **constructor** của `Logger` tại địa chỉ `storage` — không cấp phát byte nào, `storage` đã tồn tại từ trước (mảng tĩnh, đủ lớn và đúng căn lề nhờ `alignas(Logger)`). Vì không có `delete` tương ứng cho placement new, bạn phải:
1. Gọi destructor **thủ công**: `log1->~Logger();`.
2. Không gọi `delete storage` hay `delete log1` — `storage` không phải do `new` thường cấp phát.

Placement new hữu ích khi:
- Cần trì hoãn lúc gọi constructor (ví dụ: đối tượng toàn cục cần khởi tạo *sau* một bước cấu hình phần cứng, tránh vấn đề thứ tự khởi tạo tĩnh — "static initialization order fiasco").
- Muốn tái sử dụng **cùng một vùng nhớ** cho các đối tượng có vòng đời không chồng lấp (như ví dụ: `"boot"` rồi tới `"sensor"`), tránh giữ nhiều vùng tĩnh cho các đối tượng không tồn tại cùng lúc.

## Góc nhúng
- `std::pmr::monotonic_buffer_resource` không có bộ nhớ đệm ẩn ngoài `arena` bạn đưa vào — kiểm tra bằng cách tăng số phần tử `push_back` cho tới khi vượt 256 byte và quan sát chương trình dừng với `bad_alloc` (nếu build có exception) hoặc `std::terminate` (nếu `-fno-exceptions`).
- Toàn bộ kỹ thuật trong chương này **không dùng heap hệ thống**; kiểm tra lại bằng cách override `operator new` toàn cục để in cảnh báo mỗi khi bị gọi (bài tập 3) — nếu không thấy dòng in nào, xác nhận chương trình sạch heap.
- `alignas(T)` trên mảng byte tĩnh đảm bảo căn lề đúng cho `T` — thiếu nó, `placement new` trên vi kiến trúc yêu cầu căn lề nghiêm ngặt có thể gây lỗi bus fault.

## Lỗi thường gặp
- Quên gọi destructor thủ công sau placement new — rò tài nguyên mà đối tượng nắm giữ (không phải bộ nhớ `storage`, mà là thứ destructor lẽ ra dọn dẹp).
- Gọi `delete` trên con trỏ trả về từ placement new — hành vi không xác định (cố giải phóng bộ nhớ chưa từng được `new` cấp phát theo cách thường).
- Dùng `monotonic_buffer_resource` cho vòng đời dài, cấp phát/giải phóng liên tục — nó không thu hồi, vùng tĩnh sẽ đầy dần rồi thất bại dù các đối tượng "cũ" đã hết dùng.
- Quên `alignas` khi khai báo vùng nhớ tĩnh cho placement new của một kiểu có yêu cầu căn lề lớn (ví dụ chứa `double`).

## Bài tập
1. Giảm `arena` xuống 32 byte trong `pmr_static_arena.cpp` và quan sát điều gì xảy ra khi `push_back` vượt quá dung lượng.
2. Viết một `monotonic_buffer_resource` thứ hai dùng chung `arena` cho một `std::pmr::vector<char>` — giải thích vì sao hai container không nên dùng chung một resource nếu cả hai đều sống lâu và cấp phát xen kẽ.
3. Override `operator new(std::size_t)` toàn cục để in ra một dòng cảnh báo mỗi lần bị gọi, rồi chạy lại cả hai ví dụ của chương — xác nhận không dòng nào được in.
4. Viết một "pool 2 ô" bằng placement new thủ công (không dùng `std::pmr`) cho kiểu `Logger`, tương tự `BufferPool` ở chương 18 nhưng không cần `Handle` move-only — chỉ cần hàm `construct`/`destroy` nhận chỉ số ô.

## Tóm tắt
- `std::pmr` cho container chuẩn cấp phát từ vùng nhớ tĩnh thay vì heap hệ thống, không đổi API sử dụng.
- `monotonic_buffer_resource` nhanh và đơn giản nhưng không thu hồi từng phần tử — hợp với vòng đời cấp-phát-một-lần.
- Placement new xây đối tượng vào bộ nhớ có sẵn; đi kèm trách nhiệm gọi destructor thủ công, không dùng `delete`.
- Ba kỹ thuật của chương (pool tay, `std::pmr`, placement new) đều là biến thể của cùng nguyên tắc: **bộ nhớ được quyết định trước, lúc biên dịch hoặc lúc khởi động**, không phó mặc cho heap lúc chạy.
