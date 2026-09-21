// Thao tác trường bit của thanh ghi bằng hằng và hàm constexpr (thay vì bit-field, vốn phụ thuộc trình biên dịch).
#include <cstdint>
#include <cstdio>

namespace ctrl {
constexpr std::uint32_t kEnable    = 1u << 0;
constexpr unsigned      kModeShift = 1;
constexpr std::uint32_t kModeMask  = 0x3u << kModeShift;   // bit 1..2
constexpr std::uint32_t kIrqEn     = 1u << 3;
}  // namespace ctrl

constexpr std::uint32_t set_mode(std::uint32_t reg, std::uint32_t mode) {
    return (reg & ~ctrl::kModeMask) | ((mode << ctrl::kModeShift) & ctrl::kModeMask);
}
constexpr std::uint32_t get_mode(std::uint32_t reg) { return (reg & ctrl::kModeMask) >> ctrl::kModeShift; }

static_assert(get_mode(set_mode(0, 2)) == 2);
static_assert(set_mode(0xFFFFFFFFu, 0) == (0xFFFFFFFFu & ~ctrl::kModeMask));   // chỉ xoá đúng trường

int main() {
    std::uint32_t reg = 0;
    reg |= ctrl::kEnable;
    reg = set_mode(reg, 3);
    reg |= ctrl::kIrqEn;
    std::printf("reg = 0x%08X, mode = %u\n", static_cast<unsigned>(reg), static_cast<unsigned>(get_mode(reg)));

    reg = set_mode(reg, 1);
    reg &= ~ctrl::kIrqEn;
    std::printf("reg = 0x%08X, mode = %u\n", static_cast<unsigned>(reg), static_cast<unsigned>(get_mode(reg)));
    return 0;
}
