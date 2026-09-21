---
chapter: 12
title: "Class, bất biến và RAII nâng cao"
part: 3
code: code/chapter-12
---

# Chương 12. Class, bất biến và RAII nâng cao

## Mục tiêu
- Thiết kế lớp sao cho **không thể** tạo ra trạng thái sai — khái niệm "bất biến" (invariant).
- Viết một bộ đệm vòng (ring buffer) an toàn: không tràn, không đọc rác.
- Mở rộng RAII sang trường hợp lồng nhau: khoá vùng tới hạn (critical section) đếm số lần lồng.

## Câu chuyện: bộ đệm vòng bị hỏng vì viết tay
Một driver UART dùng `uint8_t buf[64]` cùng hai biến `head`, `tail` là biến toàn cục, thao tác trực tiếp từ nhiều hàm khác nhau. Một hôm có người thêm một hàm "for debug" ghi thẳng vào `buf[tail]` mà quên tăng `tail`, hoặc tăng `tail` mà quên kiểm tra tràn. Bug tồn tại nhiều tháng vì rất khó thấy: chỉ lộ ra khi bộ đệm gần đầy. Vấn đề gốc rễ: **không có gì ngăn code bên ngoài phá vỡ mối quan hệ giữa `head`, `tail` và dữ liệu**. `class` với dữ liệu `private` giải quyết đúng việc này.

## Kiến thức

### 1. Bất biến (invariant) là gì
Bất biến là điều kiện luôn đúng về trạng thái của đối tượng, tại mọi thời điểm code bên ngoài có thể nhìn thấy nó. Ví dụ với bộ đệm vòng dung lượng `Capacity`:

- `0 <= count_ <= Capacity`
- `head_` và `tail_` luôn nằm trong `[0, Capacity)`
- Nếu `count_ == 0` thì bộ đệm rỗng; nếu `count_ == Capacity` thì đầy.

**Cách đảm bảo bất biến bằng ngôn ngữ:**
1. Dữ liệu là `private`.
2. Constructor thiết lập bất biến lần đầu.
3. **Mọi** hàm public khác — nếu thay đổi trạng thái — phải giữ bất biến đúng khi kết thúc.
4. Không hàm public nào cho phép đặt trực tiếp `head_`/`tail_`/`count_` từ bên ngoài.

### 2. Bộ đệm vòng đóng gói đúng cách

{{code:chapter-12/ring_buffer.cpp}}

{{out:chapter-12/ring_buffer}}

So với biến toàn cục kiểu cũ:
- `push()`/`pop()` là **hai cửa duy nhất** để thay đổi trạng thái; cả hai đều tự kiểm tra đầy/rỗng trước khi làm gì.
- `[[nodiscard]]` bắt người gọi phải xử lý trường hợp `push` thất bại (bộ đệm đầy) — không thể "lỡ quên" như với code thủ tục.
- `advance()` là `static constexpr`: phép tính chỉ số vòng chỉ nằm ở **một chỗ**, không lặp lại ở nhiều hàm (nguồn phổ biến của lỗi sai lệch).
- Template hoá theo `T` và `Capacity`: dùng được cho bất kỳ kiểu dữ liệu, dung lượng cố định biết lúc biên dịch, không có heap.

### 3. Quy tắc 0/3/5 — rút gọn cho nhúng
C++ có "quy tắc năm" (Rule of Five): nếu một lớp cần tự viết destructor, copy constructor, copy assignment, move constructor, hoặc move assignment, thường cần viết **cả năm** (hoặc xoá bớt) để tránh hành vi ngầm sai. Với nhúng, thực dụng nhất là:

- **Quy tắc 0 (mặc định, nên nhắm tới):** không tự viết cái nào trong năm — để trình biên dịch tự sinh, đúng trong hầu hết trường hợp khi lớp chỉ chứa các thành viên "biết tự lo" (`std::array`, số nguyên, `enum class`).
- **Khi lớp sở hữu tài nguyên duy nhất** (một khoá, một kênh DMA, một chân GPIO đã cấu hình): **xoá copy** (`ClassName(const ClassName&) = delete;`) để tránh hai đối tượng cùng "sở hữu" một tài nguyên — xem `ScopedIrqLock` bên dưới.

### 4. RAII lồng nhau: khoá vùng tới hạn đếm số lần

{{code:chapter-12/critical_section.cpp}}

{{out:chapter-12/critical_section}}

Vấn đề mà `ScopedIrqLock` giải quyết: khi `update_twice()` gọi `increment_shared()` hai lần, và cả hai hàm đều "khoá vùng tới hạn", nếu khoá đơn giản bật ngắt lại ngay khi `increment_shared()` kết thúc, ngắt sẽ bị bật **giữa chừng** trong khi `update_twice()` vẫn tưởng mình đang ở vùng an toàn. Biến `depth_` (`static`, dùng chung cho mọi đối tượng của lớp) đếm số lớp lồng: chỉ tắt ngắt ở lần vào đầu tiên, chỉ bật lại ở lần ra cuối cùng — kết quả log cho thấy `irq_enabled=0` xuyên suốt cả `update_twice()`, kể cả bên trong hai lời gọi `increment_shared()`.

`ScopedIrqLock(const ScopedIrqLock&) = delete;` ngăn ai đó vô tình sao chép đối tượng khoá — sao chép ở đây vô nghĩa (hai bản "khoá" của cùng một tài nguyên không phải là hai tài nguyên độc lập).

## Ví dụ thực tiễn: vì sao không dùng biến `bool g_irq_was_enabled` viết tay
Cách viết tay thường gặp: `bool saved = irq_enabled(); irq_disable(); ...; if (saved) irq_enable();` lặp lại ở mọi hàm cần vùng tới hạn — dễ quên một nhánh `return` sớm (như `goto cleanup` ở chương 1). `ScopedIrqLock` chuyển trách nhiệm đó cho destructor: dù hàm return ở đâu, ngắt luôn được khôi phục đúng.

## Góc nhúng
- `RingBuffer<uint8_t, 64>` chiếm đúng 64 + 3×`sizeof(size_t)` byte, không có chi phí ẩn; kiểm chứng bằng `sizeof`.
- `ScopedIrqLock` không chiếm byte nào cho mỗi đối tượng (không có thành viên non-static); biến đếm `depth_` là `static inline`, chỉ tồn tại **một bản** cho cả lớp.
- Trên chip thật, `irq_disable()`/`irq_enable()` là các hàm intrinsic của toolchain (ví dụ `__disable_irq()`/`__enable_irq()` trên CMSIS) — thay `g_irq_enabled` giả lập bằng lệnh CPU thật.

## Lỗi thường gặp
- Để dữ liệu (`head_`, `tail_`, `count_`) là `public` "cho tiện debug" — phá vỡ toàn bộ lý do dùng class.
- Viết `push()` không kiểm tra `full()`, ghi đè dữ liệu chưa đọc.
- Sao chép một đối tượng khoá RAII (nếu không `= delete`), dẫn đến ngắt bị bật/tắt sai thời điểm khi bản sao bị huỷ.
- Quên rằng `static` bên trong hàm/lớp dùng **chung** cho mọi đối tượng — hữu ích ở đây (đếm lồng nhau) nhưng nguy hiểm nếu dùng nhầm cho trạng thái tưởng là "riêng của từng đối tượng".

## Bài tập
1. Thêm hàm `peek()` vào `RingBuffer` (xem phần tử đầu mà không lấy ra); đảm bảo không vi phạm bất biến.
2. Thêm `static_assert(Capacity > 0)` vào `RingBuffer` và giải thích tại sao `Capacity == 0` là vô nghĩa cho lớp này.
3. Viết một lớp `ScopedGpioHigh` bật một chân lên mức cao lúc tạo và hạ xuống thấp lúc huỷ; xử lý đúng khi lồng nhau nếu cần (gợi ý: chân GPIO có cần đếm lồng như ngắt không? Vì sao khác?).
4. Cố tình bỏ `[[nodiscard]]` khỏi `push()`, gọi `rb.push(60);` mà không kiểm tra kết quả trong một vòng lặp đẩy 10 phần tử vào bộ đệm dung lượng 4. Giải thích hậu quả im lặng của việc mất dữ liệu.
5. Giải thích bằng lời: vì sao `RingBuffer` không cần và không nên xoá copy constructor, trong khi `ScopedIrqLock` thì cần.

## Tóm tắt
- Bất biến là hợp đồng nội bộ của lớp; `private` + kiểm tra trong từng hàm public là cách chuẩn để giữ nó đúng.
- Bộ đệm vòng đóng gói trong `class` loại bỏ lớp lỗi "quên đồng bộ head/tail" của cách viết bằng biến rời rạc.
- Quy tắc 0: để trình biên dịch tự sinh copy/move khi có thể; xoá copy khi lớp sở hữu một tài nguyên duy nhất không thể chia sẻ.
- RAII lồng nhau (đếm số lần vào) giải quyết đúng bài toán "khoá được gọi từ nhiều hàm gọi nhau".
