// Kích thước kiểu phụ thuộc nền tảng; kiểu độ rộng cố định thì không.
#include <cstdint>
#include <cstdio>
#include <limits>

int main() {
    std::printf("char=%zu short=%zu int=%zu long=%zu long long=%zu void*=%zu\n",
                sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(long long), sizeof(void*));
    std::printf("uint8_t=%zu uint16_t=%zu uint32_t=%zu uint64_t=%zu\n",
                sizeof(std::uint8_t), sizeof(std::uint16_t), sizeof(std::uint32_t), sizeof(std::uint64_t));
    std::printf("uint8_t max=%d, int8_t min=%d\n",
                static_cast<int>(std::numeric_limits<std::uint8_t>::max()),
                static_cast<int>(std::numeric_limits<std::int8_t>::min()));
    return 0;
}
