// C++ nói chuyện với API kiểu C (HAL của hãng chip) qua extern "C",
// và bọc nó trong một lớp C++ an toàn hơn.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>

// Trong dự án thật, phần này nằm trong header của hãng:
//   #ifdef __cplusplus
//   extern "C" {
//   #endif
//   void hal_uart_write(const uint8_t* data, size_t len);
//   #ifdef __cplusplus
//   }
//   #endif
extern "C" void hal_uart_write(const std::uint8_t* data, std::size_t len) {
    for (std::size_t i = 0; i < len; ++i) std::putchar(data[i]);
}

// Lớp bọc C++: nhận std::span nên độ dài đi kèm dữ liệu, không thể truyền lệch.
class Uart {
public:
    void write(std::span<const std::uint8_t> data) { hal_uart_write(data.data(), data.size()); }
};

int main() {
    const std::uint8_t msg[] = {'H', 'i', '\n'};
    Uart uart;
    uart.write(msg);
    return 0;
}
