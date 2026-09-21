// Chống dội nút bấm: chỉ chấp nhận mức mới khi nó ổn định đủ N mẫu liên tiếp.
#include <cstdint>
#include <cstdio>

class Debouncer {
public:
    explicit constexpr Debouncer(std::uint8_t stable_samples) : need_(stable_samples) {}

    // Gọi định kỳ (ví dụ mỗi 1 ms). Trả về true nếu trạng thái ổn định vừa đổi.
    bool update(bool raw) {
        if (raw == stable_) { count_ = 0; return false; }
        if (++count_ >= need_) { stable_ = raw; count_ = 0; return true; }
        return false;
    }
    [[nodiscard]] constexpr bool pressed() const { return stable_; }

private:
    std::uint8_t need_;
    std::uint8_t count_ = 0;
    bool stable_ = false;
};

int main() {
    // Chuỗi mẫu có nhiễu: dội vài lần rồi ổn định ở mức 1, sau đó nhả.
    const bool samples[] = {0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0};
    Debouncer btn(4);
    int t = 0;
    for (bool s : samples) {
        bool changed = btn.update(s);
        std::printf("t=%2d raw=%d %s\n", t++, s, changed ? (btn.pressed() ? "=> NHAN" : "=> NHA") : "");
    }
    return 0;
}
