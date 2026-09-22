---
chapter: 32
title: "ETL và thư viện container/callable không heap"
part: 7
code: code/chapter-32
---

# Chương 32. Thư viện container và callable không heap

## Mục tiêu
- Hiểu vì sao STL chuẩn (`std::vector`, `std::function`) thường không phù hợp firmware cấm heap, và container/callable **sức chứa cố định** giải quyết thế nào.
- Tự viết `FixedVector<T, N>` và `FunctionRef<Sig>` để nắm cơ chế, trước khi dùng thư viện thật như ETL.
- Biết ETL (Embedded Template Library) tồn tại như một lựa chọn thư viện chuyên cho nhúng, và khi nào nên dùng nó thay vì tự viết.

## Câu chuyện: `std::vector` "rất tiện" cho tới khi profiling lộ ra `malloc`
Một driver ghép danh sách cảm biến bằng `std::vector<Sensor>`, code gọn và dễ đọc. Khi mang lên MCU thật với heap giới hạn, mỗi lần `push_back` vượt sức chứa hiện tại gây cấp phát lại (thường **gấp đôi** dung lượng) — không dự đoán được thời điểm, và có thể phân mảnh heap sau hàng giờ chạy. Giải pháp không phải "bỏ container, quay về mảng C thô" — mà là dùng một container có **API tương tự** nhưng sức chứa cố định, biết trước lúc biên dịch.

## Kiến thức

### 1. `FixedVector<T, N>`: API quen thuộc, không heap

{{code:chapter-32/fixed_vector.cpp}}

{{out:chapter-32/fixed_vector}}

`FixedVector` có `push_back`, `operator[]`, `begin`/`end` (dùng được với range-for) giống `std::vector`, nhưng dữ liệu nằm trong **mảng thành viên cố định** `T data_[N]` — không `new`, không con trỏ tới heap. Khi đầy, `push_back` trả `false` thay vì tự động cấp phát lại — người gọi **phải** xử lý trường hợp đầy, đúng tinh thần "lỗi có kiểm soát" đã lặp lại xuyên suốt sách (chương 18, 19, 22).

`sizeof(FixedVector<int, 4>)` là một con số **cố định, biết lúc biên dịch** — khác hẳn `std::vector` mà kích thước thật (bao gồm bộ nhớ heap nó trỏ tới) không nằm trong `sizeof` của chính đối tượng.

### 2. `FunctionRef<Sig>`: callable không heap, không giới hạn kiểu capture

{{code:chapter-32/function_ref.cpp}}

{{out:chapter-32/function_ref}}

Chương 16 đã nêu vấn đề của `std::function`: có thể cấp phát heap khi callable lớn. `FunctionRef` giải quyết một tình huống khác: **tham chiếu không sở hữu** tới một callable đã tồn tại sẵn (một lambda cục bộ, một functor), dùng làm tham số hàm. `sizeof(FunctionRef<int(int)>)` luôn là hai con trỏ, **bất kể** lambda bên trong lớn cỡ nào (capture nhiều biến) — vì nó không sao chép callable, chỉ giữ địa chỉ và một "trampoline" (mẫu đã gặp ở chương 28) gọi đúng kiểu thật qua template `invoke_impl<F>`.

**Đánh đổi quan trọng — vòng đời:** giống mọi tham chiếu (chương 9), `FunctionRef` chỉ hợp lệ khi đối tượng nó trỏ tới **còn sống**. Không được trả `FunctionRef` ra khỏi hàm đã tạo lambda cục bộ đó, hay lưu nó lại dùng sau khi lambda đã bị huỷ — đây là lỗi dangling reference kinh điển (chương 16 mục 3), áp dụng y hệt ở đây.

Phiên bản tối giản trong sách **chưa hỗ trợ** nhận thẳng con trỏ hàm thường (`int(*)(int)`), vì con trỏ hàm không chuyển đổi ngầm định sang `void*` (khác con trỏ đối tượng) — một implementation đầy đủ cần xử lý riêng trường hợp này (thường bằng nạp chồng constructor hoặc `reinterpret_cast` có ghi chú rõ ràng về tính khả chuyển).

### 3. ETL (Embedded Template Library) — khi nào dùng thư viện có sẵn
[ETL](https://www.etlcpp.com) là thư viện mã nguồn mở, header-only, cung cấp các container sức chứa cố định (`etl::vector`, `etl::deque`, `etl::map`...), `etl::delegate` (tương tự `FunctionRef`/`std::function` không heap), và nhiều tiện ích khác theo đúng tinh thần chương này — đã được kiểm thử kỹ và dùng rộng rãi trong công nghiệp nhúng. **Chữ ký API chính xác và cách cài đặt (CMake `FetchContent`, submodule...) nên tra tài liệu chính thức của ETL cho phiên bản bạn dùng** — sách không đưa API của ETL vào ví dụ chạy được vì môi trường biên dịch của sách không có sẵn thư viện này.

Tự viết `FixedVector`/`FunctionRef` như trong chương có giá trị: hiểu **cơ chế**, và đủ dùng cho dự án nhỏ hoặc khi không muốn thêm phụ thuộc ngoài. Với dự án lớn, nhiều kiểu container cần dùng, ETL (hoặc thư viện tương đương đã kiểm thử kỹ) thường đáng tin cậy hơn code tự viết vội.

## Góc nhúng
- Sức chứa `N` của `FixedVector` là một quyết định thiết kế **tường minh**, không phải "cứ thêm bao nhiêu cũng được" — chọn dựa trên phân tích worst-case của dữ liệu thực tế (số cảm biến tối đa, độ sâu hàng đợi lệnh...), không đoán.
- `FixedVector<T, N>` với `T` không tầm thường (có constructor/destructor riêng) gọi constructor cho **toàn bộ N phần tử** ngay khi khởi tạo (`T data_[N]{}`), kể cả các ô "chưa dùng" — với `T` nặng, cân nhắc dùng vùng nhớ thô (`std::byte`/`aligned_storage`) + placement new (chương 22) chỉ cho các ô đang dùng.
- Đo `sizeof` mọi container cố định bạn thêm vào driver — tổng cộng lại rất nhanh trên MCU RAM nhỏ.

## Lỗi thường gặp
- Trả về hoặc lưu trữ `FunctionRef` trỏ tới một lambda cục bộ đã ra khỏi phạm vi — dangling reference im lặng.
- Chọn `N` quá nhỏ cho `FixedVector` dựa trên dữ liệu thử nghiệm bàn, không dựa trên worst-case thực tế; `push_back` âm thầm thất bại trong sản phẩm nếu không kiểm tra giá trị trả về.
- Dùng `std::vector` "tạm thời cho nhanh" trong code chạy trên MCU rồi quên đổi lại trước khi release.
- Viết lại từ đầu một container phức tạp (ví dụ bản đồ băm cố định) thay vì dùng ETL hoặc thư viện tương đương đã kiểm thử kỹ, tốn thời gian và dễ có bug tinh vi.

## Bài tập
1. Thêm `insert_at(index, value)` và `erase(index)` cho `FixedVector`, dịch chuyển phần tử phù hợp.
2. Viết một `FixedVector<FunctionRef<void()>, 4>` làm danh sách "việc cần làm" (task list) đơn giản, gọi từng phần tử trong vòng lặp — lưu ý vòng đời của các callable được tham chiếu.
3. Đọc tài liệu chính thức của ETL (etlcpp.com) về `etl::vector` — so sánh API và hành vi khi đầy với `FixedVector` tự viết trong chương.
4. Giải thích bằng lời vì sao `FunctionRef` không thể (một cách an toàn và tổng quát) nhận trực tiếp một con trỏ hàm thường mà không có xử lý đặc biệt, trong khi nhận lambda/functor lại tự nhiên.

## Tóm tắt
- Container/callable sức chứa cố định giữ được API tiện lợi của STL mà không cần heap, đổi lấy việc phải xử lý tường minh trường hợp "đầy".
- `FunctionRef` là tham chiếu không sở hữu tới callable, nhẹ hơn `std::function` nhưng đòi hỏi đối tượng được tham chiếu phải còn sống khi gọi.
- ETL là lựa chọn thực tế, đã kiểm thử, cho các container/tiện ích này trong dự án nhúng thật — tự viết chỉ để hiểu cơ chế hoặc cho nhu cầu rất nhỏ.
