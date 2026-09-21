// Chuyển giá trị ADC 12 bit sang mV: cùng công thức, một bản đúng và một bản tràn số.
#include <cstdint>
#include <cstdio>

constexpr std::uint32_t kVrefMv = 3300;
constexpr std::uint32_t kMaxRaw = 4095;   // 2^12 - 1

// Đúng: nhân trong 32 bit, chia rồi mới thu hẹp về 16 bit.
constexpr std::uint16_t adc_to_mv(std::uint16_t raw) {
    return static_cast<std::uint16_t>((static_cast<std::uint32_t>(raw) * kVrefMv) / kMaxRaw);
}

// Sai: thu hẹp tích về 16 bit TRƯỚC khi chia -> mất các bit cao.
constexpr std::uint16_t adc_to_mv_bug(std::uint16_t raw) {
    std::uint16_t product = static_cast<std::uint16_t>(raw * kVrefMv);
    return static_cast<std::uint16_t>(product / kMaxRaw);
}

static_assert(adc_to_mv(4095) == 3300);
static_assert(adc_to_mv(0) == 0);

int main() {
    const std::uint16_t samples[] = {0, 1024, 2048, 4095};
    std::puts("raw  dung(mV)  sai(mV)");
    for (std::uint16_t raw : samples)
        std::printf("%4u %9u %8u\n", static_cast<unsigned>(raw),
                    static_cast<unsigned>(adc_to_mv(raw)), static_cast<unsigned>(adc_to_mv_bug(raw)));
    return 0;
}
