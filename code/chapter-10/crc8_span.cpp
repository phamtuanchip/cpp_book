// CRC-8 (đa thức 0x07) với bảng tra sinh lúc biên dịch, nhận dữ liệu qua std::span.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>

constexpr std::array<std::uint8_t, 256> make_crc8_table() {
    std::array<std::uint8_t, 256> t{};
    for (unsigned i = 0; i < 256; ++i) {
        std::uint8_t crc = static_cast<std::uint8_t>(i);
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x80) ? static_cast<std::uint8_t>((crc << 1) ^ 0x07) : static_cast<std::uint8_t>(crc << 1);
        t[i] = crc;
    }
    return t;
}
constexpr auto kCrcTable = make_crc8_table();

constexpr std::uint8_t crc8(std::span<const std::uint8_t> data) {
    std::uint8_t crc = 0;
    for (std::uint8_t b : data) crc = kCrcTable[crc ^ b];
    return crc;
}

// Giá trị kiểm tra chuẩn của CRC-8 (poly 0x07, init 0) cho chuỗi "123456789" là 0xF4.
constexpr std::array<std::uint8_t, 9> kCheck{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
static_assert(crc8(kCheck) == 0xF4, "bảng CRC sai");
static_assert(kCrcTable[1] == 0x07);

int main() {
    const std::uint8_t frame[] = {0x01, 0x02, 0x03, 0x04};
    std::printf("crc8(frame)        = 0x%02X\n", static_cast<unsigned>(crc8(frame)));
    std::printf("crc8(\"123456789\")  = 0x%02X\n", static_cast<unsigned>(crc8(kCheck)));
    // Cửa sổ con: bỏ byte đầu, không sao chép dữ liệu.
    std::printf("crc8(frame[1..])   = 0x%02X\n", static_cast<unsigned>(crc8(std::span<const std::uint8_t>(frame).subspan(1))));
    return 0;
}
