---
chapter: 36
title: "Phụ lục: Cheat sheet và tổng hợp lỗi thường gặp"
part: 8
code: code/chapter-36
---

# Phụ lục. Cheat sheet và tổng hợp lỗi thường gặp

## Mục tiêu
- Có một trang tra cứu nhanh: chọn công cụ C++ nào cho bài toán nào, không phải lật lại từng chương.
- Có danh sách tổng hợp các lỗi đã lặp lại nhiều lần xuyên suốt sách, theo chủ đề, để rà soát trước khi review code.
- Biết đi tiếp từ đâu sau khi đọc xong 35 chương.

## Cheat sheet: chọn công cụ theo bài toán

| Bài toán | Chọn | Chương |
|---|---|---|
| Kiểu số cho thanh ghi/bộ đếm | `uint8_t`/`uint16_t`/`uint32_t` tường minh, không `int`/`auto` | 7, 16 |
| Hằng số biết lúc biên dịch | `constexpr` + `static_assert` | 8, 19 |
| Mảng cố định | `std::array` | 10, 17 |
| Chuỗi/vùng nhớ chỉ đọc, không sao chép | `std::string_view` / `std::span` | 10, 17 |
| Giá trị có thể vắng mặt | `std::optional` | 17 |
| Tập sự kiện đóng, kiểm tra đủ nhánh | `std::variant` + `std::visit` | 17 |
| Lỗi có lý do, không dùng exception | `Result<T>` tự viết / `std::expected` (C++23) | 19 |
| Quản lý tài nguyên (buffer, bus, chip-select) | RAII, handle move-only, `unique_ptr` + deleter | 12, 18, 25 |
| Callback vào API kiểu C | Lambda không capture + `void* ctx`, hoặc trampoline `static` | 16, 28 |
| Callback nội bộ C++, không heap | Template `F&&`, hoặc `FunctionRef<Sig>` | 16, 32 |
| Đa hình chọn lúc chạy | Lớp cơ sở ảo | 14 |
| "Đa hình" chọn lúc biên dịch, không vtable | CRTP / template + `concept` | 15, 25 |
| Thanh ghi phần cứng | Struct `volatile` + con trỏ ép kiểu (MMIO) | 21 |
| Biến chia sẻ ISR ↔ main, đọc/ghi đơn giản | `volatile` | 21 |
| Biến chia sẻ cần đọc-sửa-ghi an toàn | `std::atomic` | 21 |
| Cấp phát không dùng heap hệ thống | Pool tĩnh / `std::pmr` + arena tĩnh / placement new | 18, 22 |
| Container/callable STL không heap | `std::pmr::vector`, tự viết `FixedVector`/`FunctionRef`, hoặc ETL | 22, 32 |
| Máy trạng thái | `enum class` + `switch`, gọi định kỳ không chặn | 26 |
| Kiểm thử logic không cần board | Tách logic khỏi I/O, mock qua `concept`/template | 25, 29 |

## Cờ biên dịch/link thường dùng cho nhúng

| Cờ | Tác dụng |
|---|---|
| `-std=c++20` | Chọn chuẩn ngôn ngữ (mặc định của sách) |
| `-Wall -Wextra` | Bật cảnh báo cơ bản — tối thiểu bắt buộc |
| `-Os` / `-O2` | Ưu tiên kích thước / ưu tiên tốc độ (chương 31) |
| `-fno-exceptions -fno-rtti` | Loại bỏ chi phí exception/RTTI khi không dùng `throw`/`dynamic_cast` |
| `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections` | Loại bỏ code/dữ liệu không dùng tới lúc link (chương 31) |
| `-flto` | Tối ưu xuyên file lúc link (chương 31) |
| `-Wl,-Map=firmware.map` | Xuất file map để soi kích thước từng ký hiệu (chương 20) |

## Tổng hợp lỗi thường gặp theo chủ đề

**Kiểu số và bộ nhớ (chương 7, 11, 20, 22)**
- Dùng `int`/`auto` cho thanh ghi/bộ đếm thay vì kiểu có độ rộng cố định.
- Giả định `sizeof(struct)` bằng tổng `sizeof` từng trường, quên đệm căn lề.
- Tràn stack do mảng cục bộ lớn hoặc đệ quy sâu, không có bảo vệ phần cứng để phát hiện.
- Dùng heap trong code cần thời gian dự đoán được, không đo phân mảnh/thời gian cấp phát tệ nhất.

**Con trỏ, tham chiếu, vòng đời (chương 9, 18, 25, 33)**
- Giữ con trỏ/tham chiếu vào phần tử container rồi thao tác làm container đổi kích thước (dangling do reallocation).
- Trả tham chiếu/con trỏ tới biến cục bộ đã ra khỏi phạm vi.
- Dùng đối tượng sau `std::move`, hoặc quên `noexcept` trên move khiến container chọn sao chép.
- Quên RAII cho tài nguyên có "trả lại" (chip-select, khoá, buffer), dựa vào kỷ luật gọi hàm dọn dẹp tay.

**`volatile`, MMIO, ngắt (chương 21, 24, 27)**
- Quên `volatile` cho thanh ghi phần cứng hoặc cờ chia sẻ với ISR — lỗi chỉ lộ ở bản tối ưu hoá.
- Dùng `volatile` cho biến cần đọc-sửa-ghi nguyên tử (cần `std::atomic`).
- Gọi hàm có thể block, cấp phát heap, hoặc ném ngoại lệ bên trong ISR.
- Quên `extern "C"` cho hàm ISR, hoặc gọi API RTOS không phải biến thể `FromISR` từ trong ngắt.

**Template, đa hình, thiết kế API (chương 14, 15, 25, 30, 31)**
- Dùng CRTP/template "vì nhanh hơn" mà chưa đo, trong khi bài toán thực ra cần chọn loại lúc chạy.
- Để logic không phụ thuộc tham số template nằm trong thân template, gây phình Flash qua nhiều instantiation.
- `concept` quá lỏng khiến lỗi kiểu sai vẫn lọt qua rồi báo lỗi khó đọc từ sâu bên trong.
- Chuyển đổi kiểu ngầm định (đặc biệt thu hẹp phạm vi) thay vì `static_cast` tường minh.

**Kiểm thử và chất lượng (chương 29, 30)**
- Viết driver gọi thẳng địa chỉ MMIO trong logic nghiệp vụ, không qua interface, khiến không test được trên host.
- Chỉ test đường vui, bỏ qua biên và trường hợp lỗi.
- Tưởng biên dịch sạch với `-Wall -Wextra` nghĩa là code an toàn theo chuẩn MISRA/AUTOSAR/CERT.

## Đi tiếp từ đâu
- **Thực hành**: chọn một board thật (STM32, ESP32, nRF52...), lặp lại các ví dụ của sách trên phần cứng thật thay vì chỉ trên host — nhiều bài học (timing, ngắt thật, điện áp) chỉ lộ ra ở đó.
- **Đọc thêm**: C++ Core Guidelines (chương 34), tài liệu chính thức của RTOS bạn chọn (chương 27), tài liệu ETL (chương 32) nếu dự án cần container/callable không heap phong phú hơn.
- **Theo dõi xu hướng**: các đề xuất safety profiles cho C++ (chương 34), hệ sinh thái Rust nhúng (chương 33), và TinyML (chương 35) đều đang phát triển nhanh — quay lại tài liệu chính thức định kỳ thay vì coi sách này là điểm dừng cuối.
- **Đóng góp lại**: nếu tìm thấy lỗi kỹ thuật, số liệu sai, hoặc ví dụ không chạy được trong sách này, đó chính xác là loại phản hồi giúp một cuốn sách viết cùng AI trở nên đáng tin cậy hơn theo thời gian.

## Tóm tắt
- Bảng công cụ và cờ biên dịch ở phụ lục này là điểm tra cứu nhanh, không thay thế việc đọc lại chương gốc khi cần hiểu **vì sao**.
- Phần lớn lỗi nhúng nghiêm trọng lặp lại quanh vài chủ đề: kiểu số/bộ nhớ, vòng đời con trỏ/tham chiếu, `volatile`/ngắt, và chuyển đổi kiểu ngầm định — rà soát đúng bốn nhóm này bắt được phần lớn vấn đề trước khi chúng lên phần cứng thật.
- C++ cho nhúng không đứng yên: theo dõi safety profiles, Rust, và TinyML để biết bức tranh sẽ thay đổi thế nào trong vài năm tới.
