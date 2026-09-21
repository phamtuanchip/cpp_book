// Căn lề (alignment) và padding: thứ tự trường quyết định kích thước struct.
#include <cstddef>
#include <cstdint>
#include <cstdio>

struct Bad  { std::uint8_t a; std::uint32_t b; std::uint8_t c; };    // a, [3 đệm], b, c, [3 đệm]
struct Good { std::uint32_t b; std::uint8_t a; std::uint8_t c; };    // b, a, c, [2 đệm]

// Header gói tin: bố trí sao cho KHÔNG có padding, và kiểm chứng bằng static_assert.
struct Header { std::uint8_t start; std::uint8_t length; std::uint16_t seq; };
static_assert(sizeof(Header) == 4, "Header có padding ngoài dự kiến");
static_assert(offsetof(Header, seq) == 2);

int main() {
    std::printf("sizeof(Bad)=%zu sizeof(Good)=%zu sizeof(Header)=%zu\n", sizeof(Bad), sizeof(Good), sizeof(Header));
    std::printf("Bad : a@%zu b@%zu c@%zu\n", offsetof(Bad, a), offsetof(Bad, b), offsetof(Bad, c));
    std::printf("Good: b@%zu a@%zu c@%zu\n", offsetof(Good, b), offsetof(Good, a), offsetof(Good, c));
    return 0;
}
