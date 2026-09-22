---
chapter: 27
title: "RTOS cơ bản với C++: task, hàng đợi, mutex"
part: 6
code: code/chapter-27
---

# Chương 27. RTOS cơ bản với C++

## Mục tiêu
- Hiểu ba khái niệm cốt lõi của RTOS (FreeRTOS, Zephyr...): task, hàng đợi (queue), mutex — qua một bản mô phỏng tối giản chạy được trên host.
- Biết những ràng buộc C++ đặc thù khi lập trình trên RTOS: cấp phát tĩnh và stack theo task, thứ tự khởi tạo toàn cục, API an toàn cho ISR.
- Biết tra đúng tài liệu chính thức thay vì đoán chữ ký API — sách này không thay thế reference manual của FreeRTOS/Zephyr.

> **Lưu ý quan trọng:** ví dụ chạy được của chương này là một **bản mô phỏng khái niệm tối giản trên host**, không phải FreeRTOS/Zephyr thật (không có MCU/toolchain RTOS trong môi trường biên dịch của sách). Tên hàm và chữ ký API RTOS thật được trích trong phần văn bản là để tham khảo cấu trúc; **luôn đối chiếu với tài liệu chính thức** cho phiên bản và cấu hình bạn dùng trước khi viết code thật.

## Câu chuyện: vòng lặp chính "biết làm mọi thứ"
Một firmware không dùng RTOS có một vòng lặp `while(true)` xử lý tuần tự: đọc cảm biến, xử lý giao thức, cập nhật màn hình, tất cả trong cùng một hàm khổng lồ. Khi cần thêm một tác vụ mới (ghi log ra thẻ SD, chậm và không đều), nó làm chậm mọi tác vụ khác vì tất cả chia sẻ chung một luồng thực thi tuần tự. RTOS giải quyết bằng cách chia thành các **task** độc lập, mỗi task có ngăn xếp riêng, được bộ lập lịch (scheduler) luân phiên chạy theo độ ưu tiên — task ghi thẻ SD chậm không còn chặn task đọc cảm biến khẩn cấp.

## Kiến thức

### 1. Ba khái niệm cốt lõi, mô phỏng trên host

{{code:chapter-27/cooperative_scheduler.cpp}}

{{out:chapter-27/cooperative_scheduler}}

- **Task**: một hàm chạy "độc lập" về mặt logic. Bản mô phỏng gọi `producer_task`/`consumer_task` lần lượt mỗi "tick" (round-robin, không ưu tiên) — RTOS thật dùng ngắt phần cứng (thường là SysTick) để **chuyển ngữ cảnh** giữa các task theo độ ưu tiên, có thể ngắt một task đang chạy để chạy task ưu tiên cao hơn (preemption).
- **Hàng đợi (queue)**: `FixedQueue` là bản rút gọn tối đa của khái niệm mà FreeRTOS gọi là `xQueueCreate`/`xQueueSend`/`xQueueReceive` — cấp phát cố định, không heap trong bản mô phỏng. Khác biệt quan trọng: hàng đợi RTOS thật cho phép task **CHỜ** (block) tới khi có chỗ trống (gửi) hoặc có dữ liệu (nhận), nhường CPU cho task khác trong lúc chờ — bản mô phỏng ở đây chỉ trả `false` ngay lập tức vì không có khái niệm "nhường CPU".
- **Mutex**: `SimpleMutex` + `Lock` (RAII, đúng mẫu chương 12) minh hoạ **hình dạng** của `xSemaphoreCreateMutex`/`xSemaphoreTake`/`xSemaphoreGive`. Vì bản mô phỏng chạy đơn luồng, nó không thể hiện được tranh chấp thật — mục đích chỉ là quen với việc **luôn** bọc mutex bằng RAII thay vì lock/unlock tay (dễ quên `unlock` trên đường lỗi, giống bài học CS ở chương 25).

### 2. API thật trông như thế nào (tham khảo, không phải code chạy được)
FreeRTOS thật (kiểm tra đúng phiên bản trong tài liệu chính thức của bạn) có dạng đại khái:

```cpp
// Tao task: ham than, ten, kich thuoc stack (theo TU, khong phai byte, tren nhieu port), do uu tien, handle ra.
xTaskCreate(vSensorTask, "Sensor", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1, nullptr);

// Hang doi: tao voi so phan tu va kich thuoc moi phan tu; gui/nhan co tham so "cho toi da bao lau" (tick).
QueueHandle_t q = xQueueCreate(4, sizeof(uint16_t));
xQueueSend(q, &sample, portMAX_DELAY);       // cho vo han neu day
xQueueReceive(q, &sample, pdMS_TO_TICKS(50)); // cho toi da 50ms

// Mutex: tao, khoa, mo khoa.
SemaphoreHandle_t mtx = xSemaphoreCreateMutex();
xSemaphoreTake(mtx, portMAX_DELAY);
xSemaphoreGive(mtx);
```

Zephyr RTOS dùng bộ API khác (`k_thread_create`, `k_msgq_put`/`k_msgq_get`, `k_mutex_lock`/`k_mutex_unlock`) với cùng ba khái niệm. **Chữ ký chính xác, đơn vị tham số (tick hay mili-giây), và các macro cấu hình (`configXXX`) thay đổi theo phiên bản và file cấu hình dự án** — đây là lý do sách không đưa các API này vào ví dụ chạy được: chúng không biên dịch được nếu thiếu đúng bộ header và file cấu hình RTOS của dự án cụ thể.

### 3. Ràng buộc C++ đặc thù khi chạy trên RTOS
- **Cấp phát tĩnh cho task/queue/mutex khi có thể**: FreeRTOS có các biến thể `xTaskCreateStatic`, `xQueueCreateStatic` nhận sẵn buffer do bạn cấp phát (mảng tĩnh/`.bss`) thay vì gọi `pvPortMalloc` (heap riêng của FreeRTOS, cấu hình được nhưng vẫn là heap) — nhất quán với nguyên tắc tránh heap của chương 20/22.
- **Kích thước stack theo từng task**: mỗi task có ngăn xếp **riêng**, kích thước khai báo lúc tạo task — đo bằng kỹ thuật watermarking (chương 20) cho từng task, không dùng một con số mặc định cho mọi task.
- **Thứ tự khởi tạo toàn cục**: các đối tượng C++ toàn cục (`static`, biến namespace) được khởi tạo **trước `main`**, tức là **trước khi scheduler chạy**. Constructor của chúng không được gọi API RTOS (task chưa tồn tại, scheduler chưa chạy) — nếu cần logic phụ thuộc RTOS, đặt trong task đầu tiên, không đặt trong constructor toàn cục (liên hệ `constinit`, chương 19).
- **API an toàn cho ISR**: mọi API RTOS gọi từ ISR phải dùng biến thể `...FromISR` (`xQueueSendFromISR`, `xSemaphoreGiveFromISR`...) — gọi nhầm API thường từ ISR là lỗi runtime nghiêm trọng, thường không được trình biên dịch phát hiện (liên hệ chương 24 mục 2 về ràng buộc ISR).
- **Tránh exception/RTTI trong task**: cùng lý do đã nêu ở chương 19/20 — nhiều cấu hình RTOS nhúng build `-fno-exceptions -fno-rtti`; kiểm tra `FreeRTOSConfig.h`/cấu hình Zephyr của dự án.

## Góc nhúng
- Độ ưu tiên task sai (task log chạy ưu tiên cao hơn task đọc cảm biến khẩn cấp) là lỗi thiết kế phổ biến hơn lỗi code — vẽ sơ đồ ưu tiên trước khi viết task.
- Đo "CPU idle time" (thời gian task rảnh chạy) là cách gián tiếp biết hệ thống có đang quá tải không — nhiều RTOS cung cấp hook đo việc này.
- `sizeof` của mỗi task control block + stack cộng dồn nhanh trên MCU RAM nhỏ; đếm tổng RAM cho tất cả task trước khi thêm task mới.

## Lỗi thường gặp
- Gọi API RTOS (tạo task, gửi queue...) trong constructor của một đối tượng toàn cục — chạy trước khi scheduler khởi động, hành vi không xác định hoặc treo.
- Gọi API RTOS "thường" (không phải `...FromISR`) từ trong ISR.
- Chọn kích thước stack cho task theo cảm tính thay vì đo watermark thực tế.
- Chia sẻ dữ liệu giữa hai task mà không qua queue/mutex, dựa vào "chắc là không trùng lúc đâu" — đúng là race condition kinh điển, chỉ lộ ra ngẫu nhiên khi tải cao.
- Dùng cấp phát động không giới hạn (`xTaskCreate` với heap RTOS mặc định) trong hệ thống chạy dài hạn mà không theo dõi phân mảnh heap của chính RTOS.

## Bài tập
1. Sửa `FixedQueue::send` trong ví dụ để khi đầy, nó **ghi đè phần tử cũ nhất** thay vì từ chối — thảo luận đây có phải hành vi mong muốn cho dữ liệu cảm biến hay không (so với dữ liệu lệnh điều khiển).
2. Thêm một "task" thứ ba tranh chấp `g_log_mutex` cùng lúc với task hiện có trong cùng một tick — quan sát `Lock` từ chối lấy khoá và log dòng "bỏ qua".
3. Tra tài liệu FreeRTOS chính thức (hoặc Zephyr) để tìm chữ ký đầy đủ của một hàm tạo queue tĩnh (`xQueueCreateStatic` hoặc tương đương Zephyr) và ghi lại các tham số nó cần so với bản động.
4. Vẽ sơ đồ (bằng lời hoặc hình) độ ưu tiên hợp lý cho một firmware có ba task: đọc cảm biến 1kHz, xử lý giao thức UART, ghi log ra thẻ SD — giải thích lựa chọn.

## Tóm tắt
- RTOS chia firmware thành các task độc lập, đồng bộ qua hàng đợi và mutex, được lập lịch theo độ ưu tiên — giải quyết vấn đề "một vòng lặp lớn làm mọi thứ".
- Nguyên tắc tránh heap, đo stack, và ràng buộc ISR của các chương trước áp dụng trực tiếp: mỗi task cần stack tĩnh đo đạc được, mọi API gọi từ ISR cần biến thể `FromISR`.
- Chữ ký API RTOS thật phụ thuộc phiên bản/cấu hình — luôn tra tài liệu chính thức, sách chỉ cho khung khái niệm.
