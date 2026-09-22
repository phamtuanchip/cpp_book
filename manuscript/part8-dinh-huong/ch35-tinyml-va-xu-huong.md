---
chapter: 35
title: "TinyML và xu hướng AI trên thiết bị nhúng"
part: 8
code: code/chapter-35
---

# Chương 35. TinyML và xu hướng AI trên thiết bị nhúng

## Mục tiêu
- Hiểu TinyML là gì: chạy suy luận (inference) mô hình học máy nhỏ **ngay trên MCU**, không gửi dữ liệu lên cloud.
- Hiểu lượng tử hoá (quantization) int8 và vì sao nó là chìa khoá để suy luận ML chạy được trên phần cứng không có FPU/heap lớn.
- Biết kiến trúc chung của TensorFlow Lite for Microcontrollers và vì sao nó dùng đúng các kỹ thuật C++ đã học (arena tĩnh — chương 22).

## Câu chuyện: gửi âm thanh liên tục lên cloud để phát hiện một từ khoá
Một thiết bị "luôn lắng nghe" (voice assistant nhỏ) cần phát hiện từ khoá đánh thức (wake word). Gửi audio thô liên tục qua mạng để một server xử lý tốn pin, tốn băng thông, và có vấn đề riêng tư nghiêm trọng (micro luôn truyền dữ liệu thô ra ngoài). Giải pháp TinyML: một mô hình rất nhỏ (vài chục KB) chạy **ngay trên MCU**, chỉ phát tín hiệu (và có thể chỉ khi đó mới gửi dữ liệu lên cloud) khi phát hiện đúng từ khoá — giảm hẳn năng lượng, băng thông, và rủi ro riêng tư.

## Kiến thức

### 1. Vì sao phải lượng tử hoá (quantization)
Mô hình học máy huấn luyện thường dùng số thực dấu phẩy động 32-bit (`float32`). Trên MCU không có FPU (Floating-Point Unit) phần cứng, mỗi phép toán `float` được **mô phỏng bằng phần mềm** — chậm hơn hàng chục lần so với phép toán số nguyên. Lượng tử hoá chuyển trọng số và dữ liệu sang `int8` (đôi khi `int16`), theo công thức affine:

```text
gia_tri_thuc ≈ (gia_tri_luong_tu - zero_point) × scale
```

`scale` (một số `float`, tính một lần lúc chuẩn bị mô hình) và `zero_point` (một số nguyên nhỏ) cho phép "nén" một khoảng giá trị thực về 256 mức `int8` — đánh đổi lấy sai số nhỏ (thường chấp nhận được cho các mô hình phân loại/phát hiện) để đổi lấy tốc độ và kích thước nhỏ hơn nhiều lần.

### 2. Suy luận bằng số nguyên: nhân-cộng-dồn (MAC) lượng tử hoá

{{code:chapter-35/quantized_inference.cpp}}

{{out:chapter-35/quantized_inference}}

Đây là một "nơ-ron" tuyến tính đơn giản nhất — trong thực tế, một mạng nơ-ron tích chập (CNN) nhỏ có hàng nghìn phép toán tương tự lặp lại, nhưng nguyên lý cốt lõi giống hệt:
- **Tích luỹ trong kiểu rộng hơn** (`int32` cho tích của các số `int8`): tích hai số 8-bit có thể tới ~16 bit, cộng dồn nhiều tích lại dễ vượt quá `int16` — đây là ứng dụng trực tiếp của nguyên tắc "chọn đúng độ rộng kiểu số" (chương 7) vào một bối cảnh cụ thể: **tràn số nguyên trong suy luận ML là lỗi âm thầm**, cho ra kết quả sai mà không có cảnh báo nào.
- **Requantize**: sau khi tích luỹ, kết quả phải quy đổi **trở lại** thang đo `int8` của tầng tiếp theo, và phải **làm tròn** chứ không cắt cụt — cắt cụt luôn lệch về phía 0 một cách có hệ thống, sai lệch đó tích luỹ rất nhanh qua nhiều tầng. Ví dụ dùng `float` cho bước nhân `scale` này để dễ đọc; bản triển khai tối ưu thật (như TensorFlow Lite Micro) dùng **nhân số nguyên cố định + dịch bit**, tránh `float` hoàn toàn.
- **Một chi tiết đáng chú ý ngay trong output**: giá trị trước khi làm tròn *lẽ ra* đúng bằng 7.5, nhưng kết quả in ra là `7` chứ không phải `8`. Lý do: `0.075` không biểu diễn được chính xác bằng `float` nhị phân (giá trị thật hơi nhỏ hơn, cỡ 7.4999...), nên phép làm tròn cho ra 7. Đây không phải lỗi của code làm tròn — đó chính là **lý do** các triển khai thật loại bỏ `float` khỏi đường suy luận: cùng một mô hình, cùng một dữ liệu, kết quả phải **giống hệt nhau** trên mọi thiết bị, và số nguyên cho điều đó còn `float` thì không đảm bảo.
- **ReLU** (cắt âm về 0, hoặc về `zero_point` sau lượng tử hoá) là hàm kích hoạt phổ biến nhất, rẻ nhất để tính (chỉ so sánh, không cần hàm siêu việt như sigmoid/tanh).

So sánh kết quả `int32`/lượng tử hoá với phép tính `float32` trực tiếp trong ví dụ cho thấy **sai số lượng tử hoá** — nhỏ trong ví dụ này, nhưng với mô hình thật, sai số tích luỹ qua nhiều tầng là lý do cần **đánh giá độ chính xác sau lượng tử hoá** trên tập dữ liệu thật trước khi triển khai, không chỉ tin vào lý thuyết.

### 3. Kiến trúc TensorFlow Lite for Microcontrollers (tham khảo)
Framework TinyML phổ biến nhất hiện nay là **TensorFlow Lite for Microcontrollers** (TFLite Micro), viết bằng C++. Vài quyết định thiết kế của nó khớp trực tiếp với các chương trước — không phải trùng hợp, mà vì cùng ràng buộc phần cứng dẫn tới cùng giải pháp:
- **Arena bộ nhớ tĩnh**: interpreter nhận một vùng nhớ cố định (`uint8_t tensor_arena[N]`) lúc khởi tạo, cấp phát mọi tensor trung gian từ đó — đúng ý tưởng `std::pmr::monotonic_buffer_resource` (chương 22), không heap hệ thống.
- **Không cấp phát động sau khi khởi tạo**: toàn bộ kích thước bộ nhớ cần thiết được tính **một lần** khi nạp mô hình, sau đó suy luận lặp lại nhiều lần không cấp phát thêm — dự đoán được thời gian và bộ nhớ, đúng nguyên tắc chương 20.
- **Không exception**: interpreter trả mã lỗi (`TfLiteStatus`), không ném ngoại lệ — đúng tinh thần chương 19.

**Chữ ký API chính xác của TFLite Micro thay đổi theo phiên bản** — sách không đưa ví dụ chạy được bằng chính framework này vì môi trường biên dịch của sách không có sẵn thư viện; tra [tài liệu chính thức](https://www.tensorflow.org/lite/microcontrollers) cho phiên bản bạn dùng.

### 4. Xu hướng khác đáng theo dõi
- **NPU tích hợp trong MCU**: một số dòng MCU mới (Cortex-M55 + Ethos-U, các chip AI nhúng chuyên dụng) có phần cứng tăng tốc phép nhân-cộng-dồn hàng loạt — code C++ điều khiển NPU thường qua thư viện vendor, tương tự HAL đã học ở chương 25.
- **Mô hình càng nhỏ, càng chuyên biệt**: xu hướng TinyML không chạy mô hình tổng quát lớn, mà huấn luyện mô hình **rất nhỏ, rất chuyên biệt** cho đúng một bài toán (phát hiện một loại rung động bất thường, một từ khoá) — kích thước Flash quyết định độ phức tạp mô hình được phép dùng.

## Góc nhúng
- Đo thời gian suy luận bằng bộ đếm chu kỳ CPU (DWT trên Cortex-M, nhắc lại chương 20) trước/sau lượng tử hoá — chênh lệch tốc độ giữa `float` mô phỏng phần mềm và `int8` phần cứng thường rất lớn trên MCU không FPU.
- Kích thước mô hình (file trọng số) cộng với code interpreter phải vừa Flash còn lại sau firmware chính — đo bằng `size` (chương 20) như mọi thành phần khác của firmware.
- Với các phép nhân số nguyên cố định (fixed-point) thay `float` ở bước requantize thật, ôn lại kỹ thuật số nguyên điểm cố định đã giới thiệu ở chương 7.

## Lỗi thường gặp
- Tích luỹ tích các số `int8`/`int16` trực tiếp trong kiểu hẹp, gây tràn số âm thầm — luôn tích luỹ trong kiểu rộng hơn đáng kể.
- Quên `zero_point` khi lượng tử hoá/giải lượng tử hoá, chỉ nhân `scale` — sai kết quả có hệ thống (lệch, không phải nhiễu ngẫu nhiên).
- Đánh giá độ chính xác mô hình chỉ bằng lý thuyết, không đo trên tập dữ liệu thật sau khi lượng tử hoá.
- Cấp phát tensor động trong vòng lặp suy luận thay vì tính trước một arena cố định lúc khởi tạo.

## Bài tập
1. Đổi trọng số/đầu vào trong ví dụ để tạo ra tràn số nếu tích luỹ bằng `int16` thay vì `int32` — quan sát (bằng tính tay hoặc chạy thử) kết quả sai khác thế nào.
2. Viết phiên bản `requantize_relu` dùng **nhân số nguyên cố định + dịch bit** thay cho `float` (gợi ý: nhân `scale` với một hằng số lớn, làm tròn về số nguyên, rồi dịch phải).
3. Tính tay sai số giữa kết quả `int8` lượng tử hoá và `float32` trong ví dụ; thử với `scale` nhỏ hơn/lớn hơn và quan sát ảnh hưởng tới sai số.
4. Tìm hiểu (qua tài liệu chính thức) kích thước arena bộ nhớ (`tensor_arena`) mà một ví dụ mẫu của TFLite Micro (như "hello_world" hoặc "micro_speech") sử dụng, và đối chiếu với RAM của một MCU cụ thể bạn quan tâm.

## Tóm tắt
- TinyML chạy suy luận ML ngay trên MCU, ưu tiên năng lượng/băng thông/riêng tư so với gửi dữ liệu lên cloud.
- Lượng tử hoá `int8` biến suy luận thành phép toán số nguyên (nhanh hơn, nhỏ hơn `float32` trên phần cứng không FPU), đổi lấy sai số cần đo đạc, không giả định.
- TFLite Micro và các framework TinyML khác dùng lại chính các kỹ thuật C++ nhúng đã học: arena tĩnh, không heap sau khởi tạo, không exception — vì cùng ràng buộc phần cứng dẫn tới cùng giải pháp.
