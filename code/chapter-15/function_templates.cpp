// Template hàm: viết một lần, dùng cho nhiều kiểu số, kiểm tra ràng buộc bằng concept (C++20).
#include <cstdint>
#include <cstdio>
#include <type_traits>

// 'concept' ràng buộc T phải là kiểu số học -> lỗi rõ ràng nếu gọi sai kiểu, thay vì
// lỗi khó đọc từ sâu trong thân hàm.
template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <Arithmetic T>
constexpr T clamp(T value, T lo, T hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

static_assert(clamp(150, 0, 100) == 100);
static_assert(clamp(-5, 0, 100) == 0);
static_assert(clamp(3.5, 0.0, 1.0) == 1.0);

// Template kết hợp non-type template parameter: giới hạn biết SẴN lúc biên dịch,
// không tốn RAM lưu lo/hi lúc chạy.
template <int Lo, int Hi>
constexpr int clamp_static(int value) {
    static_assert(Lo <= Hi, "Lo phải <= Hi");
    return clamp(value, Lo, Hi);
}
static_assert(clamp_static<0, 4095>(5000) == 4095);   // giới hạn ADC 12 bit, ví dụ

int main() {
    std::printf("clamp(150,0,100)      = %d\n", clamp(150, 0, 100));
    std::printf("clamp(-5,0,100)       = %d\n", clamp(-5, 0, 100));
    std::printf("clamp(3.5,0.0,1.0)    = %.1f\n", clamp(3.5, 0.0, 1.0));
    std::printf("clamp_static<0,4095>(5000) = %d\n", clamp_static<0, 4095>(5000));
    return 0;
}
