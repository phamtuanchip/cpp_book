// Xử lý lỗi không exception: Result<T> tối giản (C++23 có std::expected làm đúng việc này).
#include <cstdint>
#include <cstdio>
#include <utility>
#include <variant>

enum class Error : std::uint8_t { Timeout, BadCrc, Overflow };

constexpr const char* to_string(Error e) {
    switch (e) {
        case Error::Timeout: return "timeout";
        case Error::BadCrc: return "bad crc";
        case Error::Overflow: return "overflow";
    }
    return "?";
}

template <typename T>
class Result {
public:
    constexpr Result(T value) : v_(std::move(value)) {}
    constexpr Result(Error e) : v_(e) {}
    constexpr bool ok() const { return std::holds_alternative<T>(v_); }
    // get_if không ném exception: an toàn cho build -fno-exceptions (chỉ gọi khi ok() đúng).
    constexpr const T& value() const { return *std::get_if<T>(&v_); }
    constexpr Error error() const { return *std::get_if<Error>(&v_); }

private:
    std::variant<T, Error> v_;
};

Result<std::uint16_t> read_adc(bool link_up, std::uint32_t raw) {
    if (!link_up) return Error::Timeout;
    if (raw > 0xFFFF) return Error::Overflow;
    return static_cast<std::uint16_t>(raw);
}

Result<std::uint32_t> to_millivolt(std::uint16_t raw) {
    return static_cast<std::uint32_t>(raw) * 3300u / 4095u;
}

int main() {
    struct Case { bool up; std::uint32_t raw; };
    const Case cases[] = {{true, 2048}, {false, 0}, {true, 70000}};
    for (const auto& c : cases) {
        const auto r = read_adc(c.up, c.raw);
        if (!r.ok()) {
            std::printf("loi: %s\n", to_string(r.error()));
            continue;
        }
        const auto mv = to_millivolt(r.value());
        std::printf("raw=%u -> %u mV\n", static_cast<unsigned>(r.value()), static_cast<unsigned>(mv.value()));
    }
}
