---
title: "Lời tựa và cách dùng sách"
---

# Lời tựa và cách dùng sách

Phần lớn kỹ sư nhúng bắt đầu bằng C. C đủ gần phần cứng, đủ nhỏ để trình biên dịch nào cũng có. Nhưng khi firmware lớn dần — hàng chục driver, giao thức, máy trạng thái — C bắt đầu đòi hỏi quá nhiều kỷ luật thủ công: macro dài, con trỏ hàm tự chế, quản lý tài nguyên bằng `goto cleanup`.

C++ giải quyết đúng những chỗ đó mà **không bắt bạn trả giá lúc chạy** nếu biết dùng đúng phần. Sách này dạy phần đó.

## Sách dành cho ai
- Kỹ sư nhúng đang viết C, muốn chuyển sang C++ hiện đại.
- Sinh viên Điện tử, Cơ điện tử, CNTT học vi điều khiển và RTOS.
- Lập trình viên C++ desktop muốn hiểu ràng buộc RAM, Flash, real-time.

## Cách dùng sách
- Mỗi chương có thư mục `code/chapter-NN` chứa ví dụ. Bạn **chạy được trên PC**; phần thanh ghi được mô phỏng bằng biến giả.
- Khung mỗi chương: Mục tiêu → Câu chuyện → Kiến thức → Ví dụ → Góc nhúng → Lỗi thường gặp → Bài tập → Tóm tắt.
- Ô **Góc nhúng** trả lời câu hỏi: tính năng này tốn bao nhiêu RAM, Flash, chu kỳ CPU? Sách hướng dẫn bạn cách **tự đo** thay vì tin con số của người khác.

## Về việc viết sách cùng AI
Bản thảo được viết với sự hỗ trợ của AI, dưới sự duyệt của tác giả. Quy tắc dùng AI nằm trong thư mục `prompts/` của repo. Mọi ví dụ code phải biên dịch và chạy được trước khi vào sách.

## Quy ước
- Chuẩn ngôn ngữ mặc định: C++20. Biên dịch: `g++ -std=c++20 -Wall -Wextra`.
- Thuật ngữ tiếng Anh thông dụng (RAII, ISR, MMIO...) được giữ nguyên.
