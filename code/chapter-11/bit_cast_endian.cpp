// Đổi giữa float và các byte của nó mà không vi phạm quy tắc alias: std::bit_cast (C++20).
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>

int main() {
    const float f = 1.0f;
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(f);
    std::printf("1.0f = 0x%08X\n", static_cast<unsigned>(bits));

    static_assert(std::bit_cast<std::uint32_t>(1.0f) == 0x3F800000u, "giả định IEEE-754");
    std::printf("endian: %s\n", std::endian::native == std::endian::little ? "little" : "big");

    // Đọc các byte trong bộ nhớ của một số 32 bit -> lộ thứ tự byte.
    const std::uint32_t word = 0x11223344u;
    const auto bytes = std::bit_cast<std::array<std::uint8_t, 4>>(word);
    std::printf("byte[0]=0x%02X byte[3]=0x%02X\n", static_cast<unsigned>(bytes[0]), static_cast<unsigned>(bytes[3]));
    return 0;
}
