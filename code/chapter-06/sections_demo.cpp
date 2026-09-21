// Mỗi biến rơi vào một "section" khác nhau của file thực thi: .rodata, .data, .bss, .text.
#include <cstdint>
#include <cstdio>

const std::uint8_t kTable[16] = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 1, 2, 3, 4};  // hằng: Flash (.rodata)
std::uint32_t g_counter = 42;       // có giá trị khởi tạo khác 0: Flash chứa giá trị, RAM chứa biến (.data)
std::uint8_t g_buffer[256];         // khởi tạo 0: chỉ tốn RAM, không tốn Flash (.bss)

int main() {
    static int calls = 0;           // biến static cục bộ: cũng ở .bss (khởi tạo 0)
    ++calls;
    g_buffer[0] = kTable[calls & 15];
    g_counter += g_buffer[0];
    std::printf("g_counter = %u\n", static_cast<unsigned>(g_counter));
    return 0;
}
