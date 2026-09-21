// Trừu tượng "không tốn phí": cùng một phép tính, viết kiểu C và kiểu C++.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

// Kiểu C: con trỏ + độ dài (dễ truyền sai độ dài)
std::uint32_t sum_c(const std::uint8_t* p, std::size_t n) {
    std::uint32_t s = 0;
    for (std::size_t i = 0; i < n; ++i) s += p[i];
    return s;
}

// Kiểu C++: độ dài nằm trong kiểu, tính được lúc biên dịch
template <std::size_t N>
constexpr std::uint32_t sum_cpp(const std::array<std::uint8_t, N>& a) {
    std::uint32_t s = 0;
    for (auto v : a) s += v;
    return s;
}

constexpr std::array<std::uint8_t, 4> kData{1, 2, 3, 4};
static_assert(sum_cpp(kData) == 10, "kết quả tính ngay lúc biên dịch");

int main() {
    std::uint8_t raw[4] = {1, 2, 3, 4};
    std::printf("C   : %u\n", static_cast<unsigned>(sum_c(raw, 4)));
    std::printf("C++ : %u\n", static_cast<unsigned>(sum_cpp(kData)));
    return 0;
}
