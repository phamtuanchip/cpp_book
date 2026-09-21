---
chapter: 18
title: "Move semantics và smart pointer trong nhúng"
part: 4
code: code/chapter-18
---

# Chương 18. Move semantics và smart pointer trong nhúng

## Mục tiêu
- Hiểu move: chuyển **quyền sở hữu** thay vì sao chép.
- Viết kiểu move-only quản lý tài nguyên (ô buffer, bus, kênh DMA) theo RAII.
- Dùng `std::unique_ptr` với custom deleter cho tài nguyên không nằm trên heap; biết vì sao `shared_ptr` hiếm khi hợp với nhúng.

## Câu chuyện: hai nơi cùng "giải phóng" một buffer DMA
Driver nhận một buffer DMA từ pool, đưa cho tầng giao thức xử lý. Nếu buffer được sao chép con trỏ, hai nơi cùng tin mình sở hữu: một nơi trả buffer về pool, nơi kia vẫn ghi vào — DMA ghi đè dữ liệu của người khác. Bug này chỉ xuất hiện khi tải cao. Nếu kiểu buffer là **move-only**, trình biên dịch chặn việc sao chép và buộc bạn viết rõ ai giữ nó.

## Kiến thức

### 1. Move là gì
Sao chép (copy) tạo bản thứ hai; move **chuyển nội dung** sang đối tượng mới và để đối tượng cũ ở trạng thái "rỗng nhưng hợp lệ". `std::move(x)` không di chuyển gì cả — nó chỉ **ép `x` thành rvalue** để hàm move-constructor/move-assignment được chọn. Với kiểu chứa tài nguyên, move chỉ chép vài con trỏ/chỉ số và vô hiệu hoá nguồn.

Quy tắc thực dụng (Rule of Zero/Five):
- Nếu chỉ dùng các thành viên đã tự quản lý (`std::array`, số, `unique_ptr`) → **không viết gì** (Rule of Zero).
- Nếu tự quản lý một tài nguyên → viết đủ: destructor, move ctor, move assign, và `= delete` copy (Rule of Five). Đánh dấu move `noexcept`.

### 2. Handle move-only trên pool tĩnh

{{code:chapter-18/buffer_pool.cpp}}

{{out:chapter-18/buffer_pool}}

- `Handle(const Handle&) = delete` cấm sao chép: không thể có hai handle cùng một ô.
- Move ctor lấy `pool_`/`idx_` rồi đặt `o.pool_ = nullptr` — nguồn thành handle rỗng, destructor của nó không trả gì.
- Move assign phải `release()` ô đang giữ trước khi nhận ô mới, và tự bảo vệ trước `a = std::move(a)`.
- `consume(std::move(b))`: quyền sở hữu chuyển vào tham số `h`; khi `consume` kết thúc, `~Handle` trả ô về pool — không cần gọi hàm giải phóng thủ công. Sau đó `b` rỗng (ví dụ in ra "khong").
- Toàn bộ bộ nhớ là `std::array` tĩnh: **không heap**, hết ô thì `acquire()` trả handle rỗng thay vì ném exception.

### 3. `std::unique_ptr` với custom deleter

{{code:chapter-18/unique_ptr_deleter.cpp}}

{{out:chapter-18/unique_ptr_deleter}}

`unique_ptr` thường bị hiểu là "con trỏ cho `new`". Thực ra nó là **RAII cho mọi thứ có thể trả lại**: deleter tuỳ biến chạy khi `unique_ptr` ra khỏi scope. Ở đây "tài nguyên" là quyền dùng bus SPI, deleter đánh dấu bus rảnh. Không có `new`/`delete` nào. Với deleter rỗng như `SpiDeleter` (không lưu trạng thái), `sizeof(SpiLease)` thường bằng một con trỏ — chương in ra để bạn kiểm chứng; với con trỏ hàm làm deleter thì lớn hơn.

`std::make_unique` thì **có** dùng heap; trong firmware không cho phép heap, hãy dùng ví dụ pool ở mục 2 hoặc `unique_ptr` + deleter như trên.

### 4. `shared_ptr` — vì sao ít dùng
`std::shared_ptr` cần block đếm tham chiếu (thường cấp phát heap), đếm bằng phép **nguyên tử** (tốn chu kỳ, có thể cần thư viện atomic trên Cortex-M0), và làm khó việc suy luận ai giải phóng khi nào. Trong nhúng, quyền sở hữu thường rõ ràng (một chủ, cho mượn tạm) → `unique_ptr`/handle move-only + tham chiếu/`span` không sở hữu là đủ.

## Góc nhúng
- Move của handle chỉ là vài lệnh gán; với `-O2` thường biến mất sau inline. Kiểm tra bằng Compiler Explorer.
- Truyền handle **theo giá trị** kèm `std::move` để thể hiện "chuyển giao"; truyền `const Handle&`/`Handle&` khi chỉ dùng tạm.
- Nếu handle chạm vào ISR (ví dụ ISR trả buffer về pool), bảo vệ trạng thái `used_` bằng critical section hoặc cấu trúc lock-free — ví dụ chương này **chưa** an toàn cho đa luồng/ISR.

## Lỗi thường gặp
- Dùng đối tượng sau `std::move` (ngoài việc gán lại hoặc huỷ nó). Trạng thái hợp lệ nhưng không xác định về giá trị.
- Quên `noexcept` cho move → container/`variant` có thể chọn sao chép hoặc không dùng move.
- `std::move` một biến `const` — không có move nào xảy ra, âm thầm rơi về copy (hoặc lỗi).
- Move assign quên `release()` tài nguyên cũ → rò ô pool.
- Tạo `unique_ptr` bằng `new` rồi tưởng là "không heap".

## Bài tập
1. Thêm `size()` và `reset()` cho `Handle`; kiểm tra `reset()` trả ô về pool ngay.
2. Cố tình sao chép một `Handle` và đọc thông báo lỗi biên dịch.
3. Viết `SpiLease` thứ hai dùng deleter là **lambda** hoặc con trỏ hàm; so sánh `sizeof`.
4. Giải thích vì sao ví dụ `BufferPool` chưa an toàn khi ISR gọi `release()`, và phác thảo cách sửa.

## Tóm tắt
- Move chuyển quyền sở hữu; kiểu move-only làm lỗi "hai chủ, một tài nguyên" thành lỗi biên dịch.
- RAII + move giải phóng tài nguyên đúng lúc mà không cần heap; `unique_ptr` với custom deleter tổng quát hoá ý này.
- `shared_ptr` hiếm khi hợp với nhúng; ưu tiên một chủ rõ ràng.
