// Cấu hình và bảng tra tính lúc biên dịch thay cho macro và mảng gõ tay.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

constexpr std::uint32_t kClockHz = 16'000'000;
constexpr std::uint32_t kBaud    = 115'200;

constexpr std::uint32_t uart_divisor(std::uint32_t clk, std::uint32_t baud) {
    return clk / (16 * baud);
}
static_assert(uart_divisor(kClockHz, kBaud) == 8, "chia nguyên: 8 (xem chương sau về sai số baud)");

template <unsigned Bits>
constexpr std::uint32_t max_value() {
    static_assert(Bits >= 1 && Bits <= 32, "Bits phải nằm trong 1..32");
    return static_cast<std::uint32_t>((std::uint64_t{1} << Bits) - 1);
}
static_assert(max_value<8>()  == 255u);
static_assert(max_value<12>() == 4095u);   // ADC 12 bit

constexpr std::array<std::uint16_t, 8> make_squares() {
    std::array<std::uint16_t, 8> t{};
    for (std::size_t i = 0; i < t.size(); ++i) t[i] = static_cast<std::uint16_t>(i * i);
    return t;
}
constexpr auto kSquares = make_squares();   // nằm trong Flash, không tốn chu kỳ CPU

int main() {
    std::printf("divisor = %u\n", static_cast<unsigned>(uart_divisor(kClockHz, kBaud)));
    std::printf("ADC 12 bit max = %u\n", static_cast<unsigned>(max_value<12>()));
    std::printf("squares[7] = %u\n", static_cast<unsigned>(kSquares[7]));
    return 0;
}
