// std::array (kích thước cố định, không heap) và std::optional (thay cho giá trị "đặc biệt" như -1).
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string_view>

std::optional<std::uint16_t> parse_adc(std::string_view s) {
    if (s.empty() || s.size() > 4) return std::nullopt;
    std::uint16_t v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return std::nullopt;
        v = static_cast<std::uint16_t>(v * 10 + (c - '0'));
    }
    return v;
}

// Trả về chỉ số phần tử đầu tiên vượt ngưỡng, hoặc nullopt nếu không có.
std::optional<std::size_t> first_above(const std::array<std::uint16_t, 6>& a, std::uint16_t limit) {
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i] > limit) return i;
    return std::nullopt;
}

int main() {
    const std::array<std::uint16_t, 6> samples{120, 340, 90, 2050, 400, 15};

    if (auto idx = first_above(samples, 2000)) {
        std::printf("vuot nguong tai [%zu] = %u\n", *idx, static_cast<unsigned>(samples[*idx]));
    }
    if (!first_above(samples, 5000)) std::puts("khong co mau nao vuot 5000");

    for (std::string_view text : {"1234", "12a4", "", "99999"}) {
        const int len = static_cast<int>(text.size());
        if (auto v = parse_adc(text)) std::printf("'%.*s' -> %u\n", len, text.data(), static_cast<unsigned>(*v));
        else std::printf("'%.*s' -> loi\n", len, text.data());
    }

    std::printf("sizeof(array<u16,6>)=%zu sizeof(optional<u8>)=%zu sizeof(optional<u32>)=%zu\n",
                sizeof(samples), sizeof(std::optional<std::uint8_t>), sizeof(std::optional<std::uint32_t>));
}
