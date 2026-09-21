// enum của C tự chuyển thành int; enum class thì không, và chọn được kiểu nền.
#include <cstdio>
#include <type_traits>

enum Color { Red, Green };                      // kiểu C
enum class Level : unsigned char { Low, High }; // kiểu C++11

static_assert(std::is_convertible_v<Color, int>,   "enum C chuyển ngầm sang int");
static_assert(!std::is_convertible_v<Level, int>,  "enum class KHÔNG chuyển ngầm");
static_assert(sizeof(Level) == 1,                  "kiểu nền unsigned char -> 1 byte");

int main() {
    int a = Green;                                  // hợp lệ, dễ gây nhầm
    // int b = Level::High;                         // lỗi biên dịch: cần static_cast
    int b = static_cast<int>(Level::High);          // phải nói rõ ý định
    std::printf("Green=%d Level::High=%d sizeof(Level)=%zu\n", a, b, sizeof(Level));
    return 0;
}
