// Ring buffer khong khoa (lock-free) cho MOT nguoi ghi (ISR) va MOT nguoi doc (vong lap chinh).
// Voi dung mot writer + dung mot reader, atomic voi memory_order phu hop la du, khong can mutex/khoa ngat.
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>

template <typename T, std::size_t N>
class RingBuffer {
    static_assert((N & (N - 1)) == 0, "N nen la luy thua cua 2 de phep '&' thay the '%' (nhanh hon tren MCU)");

public:
    // Goi TU ISR: khong bao gio block, khong cap phat, that bai am tham neu day (mat byte co kiem soat
    // - tot hon la treo he thong; driver thuc te nen dem so byte bi mat de chan doan).
    bool push(const T& value) {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = static_cast<std::size_t>(head + 1) & (N - 1);
        if (next == tail_.load(std::memory_order_acquire)) return false;  // day
        buf_[head] = value;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Goi TU vong lap chinh (khong phai ISR): lay ra mot phan tu neu co.
    bool pop(T& out) {
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) return false;  // rong
        out = buf_[tail];
        tail_.store(static_cast<std::size_t>(tail + 1) & (N - 1), std::memory_order_release);
        return true;
    }

    std::size_t size() const {
        const auto h = head_.load(std::memory_order_acquire);
        const auto t = tail_.load(std::memory_order_acquire);
        return static_cast<std::size_t>(h - t) & (N - 1);
    }

private:
    std::array<T, N> buf_{};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};

static RingBuffer<std::uint8_t, 8> g_rx_buffer;  // N=8: luy thua cua 2

// Gia lap ISR nhan UART: phan cung "day" tung byte vao. Trong firmware that, ham nay
// (hoac ham no goi toi) la than cua USART1_IRQHandler (xem vi du extern_c_isr.cpp).
void simulated_uart_rx_isr(std::uint8_t byte) {
    if (!g_rx_buffer.push(byte)) {
        std::puts("CANH BAO: ring buffer day, mat 1 byte (can xu ly vong lap chinh nhanh hon hoac tang N)");
    }
}

int main() {
    const std::uint8_t frame[] = {'H', 'E', 'L', 'L', 'O'};
    for (std::uint8_t b : frame) simulated_uart_rx_isr(b);

    std::printf("ring buffer dang giu %zu byte\n", g_rx_buffer.size());

    std::uint8_t byte;
    std::printf("doc ra: ");
    while (g_rx_buffer.pop(byte)) std::printf("%c", static_cast<char>(byte));
    std::printf("\n");

    // Lap day buffer de minh hoa canh bao mat du lieu.
    for (int i = 0; i < 10; ++i) simulated_uart_rx_isr('X');
}
