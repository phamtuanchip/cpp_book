// Bộ đệm vòng (ring buffer) đóng gói bất biến (invariant) bên trong lớp: người dùng ngoài
// không thể tự tay đặt head/tail sai làm hỏng cấu trúc dữ liệu.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

template <typename T, std::size_t Capacity>
class RingBuffer {
public:
    // Bất biến của lớp: 0 <= count_ <= Capacity; head_/tail_ luôn nằm trong [0, Capacity).
    [[nodiscard]] bool push(const T& value) {
        if (full()) return false;               // không bao giờ ghi đè khi đầy
        buf_[tail_] = value;
        tail_ = advance(tail_);
        ++count_;
        return true;
    }

    [[nodiscard]] bool pop(T& out) {
        if (empty()) return false;
        out = buf_[head_];
        head_ = advance(head_);
        --count_;
        return true;
    }

    [[nodiscard]] constexpr bool empty() const { return count_ == 0; }
    [[nodiscard]] constexpr bool full() const { return count_ == Capacity; }
    [[nodiscard]] constexpr std::size_t size() const { return count_; }
    static constexpr std::size_t capacity() { return Capacity; }

private:
    static constexpr std::size_t advance(std::size_t i) { return (i + 1) % Capacity; }

    std::array<T, Capacity> buf_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t count_ = 0;
};

int main() {
    RingBuffer<std::uint8_t, 4> rb;

    for (std::uint8_t v : {10, 20, 30, 40, 50}) {   // phần tử thứ 5 sẽ tràn (capacity = 4)
        bool ok = rb.push(v);
        std::printf("push(%3u) -> %s  size=%zu\n", static_cast<unsigned>(v), ok ? "ok" : "FULL", rb.size());
    }

    std::uint8_t out = 0;
    while (rb.pop(out)) std::printf("pop() -> %u\n", static_cast<unsigned>(out));

    if (!rb.pop(out)) std::puts("pop() -> EMPTY (dung nhu mong doi)");
    return 0;
}
