// Lambda làm callback: không capture -> thành con trỏ hàm (dùng được với API kiểu C);
// có capture -> truyền qua template, không cấp phát heap, có thể inline.
#include <cstddef>
#include <cstdint>
#include <cstdio>

using RxCallback = void (*)(void* ctx, std::uint8_t byte);

class Uart {
public:
    void on_rx(RxCallback cb, void* ctx) { cb_ = cb; ctx_ = ctx; }
    void simulate_rx(std::uint8_t b) { if (cb_) cb_(ctx_, b); }
private:
    RxCallback cb_ = nullptr;
    void* ctx_ = nullptr;
};

// F được suy ra là kiểu lambda cụ thể: gọi trực tiếp, không con trỏ hàm, không std::function.
template <typename F>
void for_each_byte(const std::uint8_t* data, std::size_t n, F&& f) {
    for (std::size_t i = 0; i < n; ++i) f(data[i]);
}

int main() {
    Uart uart;
    std::uint32_t rx_count = 0;

    // Lambda không capture: chuyển ngầm sang RxCallback. Trạng thái đi qua 'ctx'.
    uart.on_rx(
        [](void* ctx, std::uint8_t b) {
            auto* count = static_cast<std::uint32_t*>(ctx);
            ++*count;
            std::printf("rx 0x%02X (thu %u)\n", static_cast<unsigned>(b), static_cast<unsigned>(*count));
        },
        &rx_count);
    uart.simulate_rx(0x41);
    uart.simulate_rx(0x42);

    // Lambda có capture theo tham chiếu, truyền vào template.
    const std::uint8_t frame[] = {0x01, 0x02, 0x03, 0x04};
    std::uint32_t sum = 0;
    for_each_byte(frame, sizeof frame, [&sum](std::uint8_t b) { sum += b; });
    std::printf("tong = %u\n", static_cast<unsigned>(sum));

    // Generic lambda (auto) và structured binding.
    auto max_of = [](auto a, auto b) { return a > b ? a : b; };
    std::printf("max = %d, %.1f\n", max_of(3, 7), max_of(2.5, 1.5));

    struct Limits { int lo; int hi; };
    const Limits lim{10, 90};
    const auto [lo, hi] = lim;
    std::printf("lo=%d hi=%d\n", lo, hi);

    auto no_capture = [](int x) { return x + 1; };
    auto with_ref = [&sum](int x) { return x + static_cast<int>(sum); };
    std::printf("sizeof(no_capture)=%zu sizeof(with_ref)=%zu\n", sizeof no_capture, sizeof with_ref);
    std::printf("%d %d\n", no_capture(1), with_ref(1));
}
