---
chapter: 30
title: "Phân tích tĩnh và các chuẩn an toàn: MISRA C++, AUTOSAR, CERT"
part: 7
code: code/chapter-30
---

# Chương 30. Phân tích tĩnh và các chuẩn an toàn

## Mục tiêu
- Biết phân tích tĩnh (static analysis) là gì, khác gì với biên dịch thông thường và với unit test (chương 29).
- Nắm tinh thần chung của các chuẩn MISRA C++, AUTOSAR C++14, CERT C++ — không sao chép quy tắc gốc (đây là tài liệu có bản quyền), mà hiểu **loại vấn đề** chúng nhắm tới.
- Áp dụng một số mẫu mã "an toàn hơn" phổ biến mà các công cụ/chuẩn này thường yêu cầu.

> **Lưu ý về bản quyền:** MISRA C++, AUTOSAR C++14 và CERT C++ Coding Standard là tài liệu chính thức có bản quyền, đánh số quy tắc cụ thể (ví dụ "Rule A5-0-1"). Sách này **không sao chép nguyên văn** nội dung quy tắc — nếu dự án của bạn bắt buộc tuân thủ một chuẩn, hãy mua/tra tài liệu gốc và dùng công cụ phân tích tĩnh có hỗ trợ đúng bộ quy tắc đó (nhiều công cụ thương mại quảng cáo "MISRA-compliant checking").

## Câu chuyện: code biên dịch sạch, không nghĩa là code an toàn
`-Wall -Wextra` không cảnh báo gì với đoạn code đọc một biến chưa chắc đã được gán trong mọi nhánh, hay với một phép chuyển đổi kiểu ngầm định làm mất dữ liệu trong một số trường hợp hiếm. Trình biên dịch tối ưu cho việc **sinh mã đúng theo những gì bạn viết**; nó không đọc ý định của bạn. Phân tích tĩnh là một lớp công cụ **khác**, quét mã nguồn để tìm các mẫu **dễ gây lỗi** dù cú pháp hoàn toàn hợp lệ — và các chuẩn như MISRA/AUTOSAR/CERT là tập hợp quy tắc đã được ngành công nghiệp (ô tô, hàng không, y tế) đúc kết qua nhiều thập kỷ sự cố thực tế.

## Kiến thức

### 1. Phân tích tĩnh khác gì trình biên dịch và unit test
| | Trình biên dịch (`-Wall -Wextra`) | Phân tích tĩnh (clang-tidy, cppcheck, PC-lint...) | Unit test (chương 29) |
|---|---|---|---|
| Chạy khi nào | Mỗi lần build | Riêng, thường trong CI, có thể chậm hơn build | Mỗi lần build/CI |
| Tìm gì | Lỗi cú pháp, một số cảnh báo phổ biến | Mẫu mã rủi ro (biến chưa khởi tạo trên một số đường, kiểu chuyển đổi nguy hiểm, vi phạm quy tắc chuẩn cụ thể) | Hành vi sai so với kỳ vọng đã viết ra |
| Cần chạy chương trình? | Không | Không (phân tích mã nguồn/AST) | Có |
| Bắt được logic sai? | Không | Một phần (một số công cụ có phân tích luồng dữ liệu) | Có, nếu có test đúng |

Ba lớp công cụ **bổ sung** cho nhau, không thay thế nhau.

### 2. Tinh thần chung của MISRA C++/AUTOSAR/CERT (không phải danh sách đầy đủ)
Các chuẩn này khác nhau về phạm vi (MISRA hướng ô tô, AUTOSAR C++14 dựa trên MISRA nhưng bổ sung cho kiến trúc AUTOSAR, CERT hướng bảo mật tổng quát hơn), nhưng nhiều quy tắc xoay quanh vài chủ đề lặp lại:
- **Loại bỏ hành vi không xác định (undefined behavior)**: tràn số nguyên có dấu, đọc biến chưa khởi tạo, con trỏ lơ lửng — những thứ chuẩn C++ "không đảm bảo gì", nên hạn chế nghiêm ngặt cách viết dẫn tới chúng.
- **Chuyển đổi kiểu tường minh, không ngầm định**: đặc biệt các chuyển đổi có thể **mất dữ liệu** (thu hẹp phạm vi số, dấu sang không dấu) phải được viết rõ bằng `static_cast`, để người đọc (và công cụ) thấy ngay đây là chuyển đổi **có chủ đích**.
- **Luồng điều khiển đơn giản, dễ chứng minh đúng**: hạn chế hoặc cấm `goto`, giới hạn độ phức tạp lồng nhau, đôi khi yêu cầu mỗi hàm chỉ có một điểm thoát (**lưu ý: bản thân yêu cầu này khác nhau giữa các phiên bản chuẩn — luôn tra đúng phiên bản bạn áp dụng**).
- **Hạn chế các tính năng ngôn ngữ "nguy hiểm nếu dùng sai"**: ví dụ nhiều chuẩn hạn chế nghiêm ngặt (không cấm hoàn toàn) đa kế thừa phức tạp, ép kiểu kiểu C (`(int)x` thay vì `static_cast<int>(x)`), hoặc yêu cầu lý do rõ ràng khi dùng con trỏ thô thay vì smart pointer/tham chiếu.
- **Không có số/chuỗi "ma thuật"**: hằng số phải có tên (`kMaxRetries` thay vì rải số `3` khắp nơi) để ý nghĩa rõ ràng và dễ sửa tại một chỗ.

### 3. Áp dụng một số mẫu "an toàn hơn"

{{code:chapter-30/safe_patterns.cpp}}

{{out:chapter-30/safe_patterns}}

- `classify_bad_style_fixed`: khởi tạo `result` **ngay lúc khai báo** với một giá trị mặc định hợp lý, thay vì để "chưa xác định" cho tới khi một nhánh `if` gán nó — loại bỏ khả năng đọc biến chưa khởi tạo nếu sau này có ai thêm một nhánh mới mà quên gán.
- `clamp_to_byte`: mọi chuyển đổi thu hẹp (`int` → `uint8_t`) đều qua `static_cast` **tường minh**, sau khi đã kiểm tra biên — không có chuyển đổi ngầm định nào có thể âm thầm cắt mất bit.
- `validate_frame`: dùng `return` sớm cho từng điều kiện lỗi, không có `goto`, không lồng `if` sâu — luồng điều khiển đọc từ trên xuống, mỗi dòng là một điều kiện loại trừ rõ ràng.
- `kMaxRetries`/`kTimeoutMs`: hằng số có tên ở phạm vi file, dùng lại ở nhiều nơi — sửa một chỗ, không tìm-và-thay số `3`/`500` rải rác.

## Góc nhúng
- Chạy `cppcheck --enable=all path/to/file.cpp` hoặc `clang-tidy file.cpp -- -std=c++20` để thấy công cụ phân tích tĩnh thật báo gì trên code của bạn — cài đặt và chữ ký dòng lệnh chính xác nên tra tài liệu công cụ, có thể khác theo phiên bản.
- Nhiều đội nhúng chạy phân tích tĩnh như một bước bắt buộc trong CI (song song với `npm run verify`/unit test của sách này), chặn merge nếu có vi phạm mức nghiêm trọng.
- Chứng nhận an toàn chức năng (ISO 26262 cho ô tô, DO-178C cho hàng không...) thường **yêu cầu** bằng chứng tuân thủ một tập quy tắc coding cụ thể — đây là lý do các chuẩn này quan trọng về mặt pháp lý/quy trình, không chỉ về chất lượng code.

## Lỗi thường gặp
- Tưởng "biên dịch sạch với `-Wall -Wextra`" nghĩa là "an toàn" — hai việc khác nhau, như bảng ở mục 1.
- Áp dụng chuẩn (MISRA...) một cách máy móc mà không hiểu **vì sao** quy tắc tồn tại, dẫn tới code "tuân thủ nhưng khó đọc hơn" thay vì an toàn hơn.
- Trích dẫn số hiệu quy tắc MISRA/AUTOSAR/CERT từ trí nhớ mà không đối chiếu tài liệu gốc — số hiệu và nội dung có thể đã đổi giữa các phiên bản.
- Chạy phân tích tĩnh một lần rồi bỏ qua, không tích hợp vào CI để nó áp dụng liên tục cho code mới.

## Bài tập
1. Cài `cppcheck` (hoặc dùng bản online nếu có) và chạy trên một file bất kỳ trong `code/chapter-01` đến `chapter-05` của sách — ghi lại cảnh báo (nếu có) và đánh giá có hợp lý không.
2. Viết một hàm cố tình vi phạm mục "luôn khởi tạo biến" (biến chưa gán trên một nhánh `if`), biên dịch với `-Wall -Wextra -Wuninitialized` (hoặc `-O2` để kích hoạt phân tích luồng dữ liệu tốt hơn) và xem trình biên dịch có bắt được không.
3. Tìm hiểu (qua tài liệu chính thức, không phải trí nhớ) sự khác biệt giữa MISRA C++ 2008 và MISRA C++:2023 về quy tắc "một điểm thoát mỗi hàm" — ghi lại tóm tắt.
4. Refactor một đoạn code có nhiều số "ma thuật" trong dự án của bạn (nếu có) thành hằng số có tên; đánh giá code có dễ đọc hơn không.

## Tóm tắt
- Phân tích tĩnh là một lớp công cụ riêng, bổ sung cho trình biên dịch và unit test, tìm mẫu mã rủi ro thay vì lỗi cú pháp hay hành vi sai cụ thể.
- MISRA C++/AUTOSAR/CERT là các chuẩn có bản quyền, đúc kết kinh nghiệm ngành; tinh thần chung xoay quanh loại bỏ hành vi không xác định, chuyển đổi kiểu tường minh, luồng điều khiển đơn giản.
- Không cần thuộc số hiệu quy tắc để bắt đầu viết an toàn hơn: khởi tạo mọi biến, `static_cast` tường minh, tránh `goto`, đặt tên cho hằng số là những bước thực dụng đầu tiên.
