// FixedVector<T, N>: API giong std::vector (push_back, size, operator[], range-for) nhung SUC CHUA
// CO DINH, khong heap - dung tinh than voi thu vien ETL (Embedded Template Library, etlcpp.com) that,
// vien viet don gian de day du hieu co che, khong thay the ETL cho du an that.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>

template <typename T, std::size_t N>
class FixedVector {
public:
    bool push_back(const T& value) {
        if (size_ == N) return false;  // day: khong cap phat them, bao that bai co kiem soat
        data_[size_++] = value;
        return true;
    }
    template <typename... Args>
    bool emplace_back(Args&&... args) {
        if (size_ == N) return false;
        data_[size_++] = T(std::forward<Args>(args)...);
        return true;
    }
    void pop_back() { if (size_ > 0) --size_; }

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    std::size_t size() const { return size_; }
    constexpr std::size_t capacity() const { return N; }
    bool full() const { return size_ == N; }
    bool empty() const { return size_ == 0; }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

private:
    T data_[N]{};
    std::size_t size_ = 0;
};

struct Reading {
    std::uint16_t raw;
    float celsius;
};

int main() {
    FixedVector<int, 4> nums;
    for (int v : {10, 20, 30}) nums.push_back(v);

    std::printf("size=%zu capacity=%zu full=%s\n", nums.size(), nums.capacity(), nums.full() ? "co" : "khong");
    for (int v : nums) std::printf("%d ", v);
    std::printf("\n");

    if (!nums.push_back(40)) std::puts("push thu 4 thanh cong");
    if (!nums.push_back(50)) std::puts("push thu 5 THAT BAI (da day, dung nhu mong doi)");

    FixedVector<Reading, 2> readings;
    readings.emplace_back(Reading{2048, 25.5f});
    readings.emplace_back(Reading{2100, 26.1f});
    for (const auto& r : readings) std::printf("raw=%u -> %.1f C\n", static_cast<unsigned>(r.raw), r.celsius);

    std::printf("sizeof(FixedVector<int,4>) = %zu (co dinh, biet duoc luc bien dich)\n", sizeof(nums));
}
