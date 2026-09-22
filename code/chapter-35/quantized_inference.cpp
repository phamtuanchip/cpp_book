// Mot "no-ron" tuyen tinh luong tu hoa int8 (dot product + bias + kich hoat ReLU), theo dung tinh
// than TinyML (TensorFlow Lite for Microcontrollers): tinh toan CHI bang so nguyen, khong co dau phay
// dong nao trong duong suy luan chinh - phu hop MCU khong co FPU hoac muon toc do/nang luong tot hon.
#include <array>
#include <cstdint>
#include <cstdio>

// Luong tu hoa affine don gian: gia_tri_thuc = (gia_tri_int8 - zero_point) * scale.
struct QuantParams {
    float scale;
    std::int8_t zero_point;
};

// Tich luy trong int32 (khong phai int8/int16) de tranh TRAN SO khi cong don nhieu tich int8*int8 -
// day la ly do TFLite Micro luon dung buffer tich luy rong hon kieu du lieu dau vao.
std::int32_t quantized_dot(const std::array<std::int8_t, 4>& input, const std::array<std::int8_t, 4>& weights,
                            QuantParams in_q, QuantParams w_q) {
    std::int32_t acc = 0;
    for (std::size_t i = 0; i < input.size(); ++i) {
        const std::int32_t x = static_cast<std::int32_t>(input[i]) - in_q.zero_point;
        const std::int32_t w = static_cast<std::int32_t>(weights[i]) - w_q.zero_point;
        acc += x * w;  // tich hai so <= 8 bit, ket qua toi da ~16 bit; cong don 4 lan van an toan trong int32
    }
    return acc;
}

// Requantize: dua tong int32 (theo thang do scale_in * scale_w) ve lai int8 theo thang do dau ra,
// roi ap ReLU (cat am ve 0) - dung mau TFLite Micro (tinh toan bang so nguyen, khong dung float o day
// ngoai duy nhat buoc quy doi thang do nay - ban toi uu that dung nhan so nguyen co dinh, khong float).
std::int8_t requantize_relu(std::int32_t acc, float scale_in, float scale_w, QuantParams out_q) {
    const float real_value = static_cast<float>(acc) * scale_in * scale_w;
    const float requantized = real_value / out_q.scale + static_cast<float>(out_q.zero_point);
    const float relu = requantized > static_cast<float>(out_q.zero_point) ? requantized : static_cast<float>(out_q.zero_point);
    const float clamped = relu > 127.0f ? 127.0f : (relu < -128.0f ? -128.0f : relu);
    return static_cast<std::int8_t>(clamped);
}

int main() {
    // Dau vao va trong so da duoc luong tu hoa san (vd boi cong cu huan luyen/luong tu hoa offline).
    const std::array<std::int8_t, 4> input{20, -5, 40, 10};
    const std::array<std::int8_t, 4> weights{3, -2, 1, 4};
    const QuantParams in_q{0.05f, 0};   // scale=0.05, zero_point=0 (du lieu dau vao da chuan hoa quanh 0)
    const QuantParams w_q{0.01f, 0};
    const QuantParams out_q{0.1f, 0};

    const std::int32_t acc = quantized_dot(input, weights, in_q, w_q);
    const std::int8_t output = requantize_relu(acc, in_q.scale, w_q.scale, out_q);

    std::printf("tich luy (int32) = %d\n", static_cast<int>(acc));
    std::printf("gia tri thuc uoc tinh = %.4f\n", static_cast<double>(acc) * in_q.scale * w_q.scale);
    std::printf("dau ra sau requantize + ReLU (int8) = %d\n", static_cast<int>(output));

    std::printf("\nSo sanh: cung phep toan bang float32 truc tiep:\n");
    float float_acc = 0.0f;
    for (std::size_t i = 0; i < input.size(); ++i)
        float_acc += static_cast<float>(input[i]) * in_q.scale * static_cast<float>(weights[i]) * w_q.scale;
    std::printf("float32: %.4f (so voi uoc tinh tu int32 o tren, chenh lech la sai so luong tu hoa)\n",
                static_cast<double>(float_acc));
}
