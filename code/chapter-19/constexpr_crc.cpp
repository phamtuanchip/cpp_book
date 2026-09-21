// Bảng CRC-8 (poly 0x07) tính hoàn toàn lúc biên dịch; static_assert kiểm tra với vector chuẩn "123456789" -> 0xF4.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

constexpr std::array<std::uint8_t, 256> make_crc8_table(std::uint8_t poly) {
    std::array<std::uint8_t, 256> table{};
    for (unsigned i = 0; i < 256; ++i) {
        auto c = static_cast<std::uint8_t>(i);
        for (int bit = 0; bit < 8; ++bit)
            c = (c & 0x80) ? static_cast<std::uint8_t>((c << 1) ^ poly) : static_cast<std::uint8_t>(c << 1);
        table[i] = c;
    }
    return table;
}

inline constexpr auto kCrcTable = make_crc8_table(0x07);

constexpr std::uint8_t crc8(const std::uint8_t* data, std::size_t n) {
    std::uint8_t crc = 0;
    for (std::size_t i = 0; i < n; ++i) crc = kCrcTable[crc ^ data[i]];
    return crc;
}

inline constexpr std::array<std::uint8_t, 9> kCheck{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
static_assert(crc8(kCheck.data(), kCheck.size()) == 0xF4, "CRC-8 sai");

// if constexpr: nhánh không dùng bị loại bỏ hoàn toàn khỏi code sinh ra.
template <typename T>
constexpr const char* width_name() {
    if constexpr (sizeof(T) == 1) return "8-bit";
    else if constexpr (sizeof(T) == 2) return "16-bit";
    else return "32-bit+";
}

int main() {
    const std::uint8_t msg[] = {0xDE, 0xAD, 0xBE, 0xEF};
    std::printf("crc8(check) = 0x%02X\n", static_cast<unsigned>(crc8(kCheck.data(), kCheck.size())));
    std::printf("crc8(msg)   = 0x%02X\n", static_cast<unsigned>(crc8(msg, sizeof msg)));
    std::printf("%s %s %s\n", width_name<std::uint8_t>(), width_name<std::uint16_t>(), width_name<std::uint32_t>());
}
