// const đúng chỗ: con trỏ tới const, con trỏ const, tham chiếu const, tham số ra.
#include <cstdint>
#include <cstdio>

struct Reading { std::int16_t temp_c10; std::uint16_t humidity_p10; };   // nhiệt độ x10, độ ẩm x10

// Truyền struct bằng tham chiếu const: không sao chép, không sửa được.
void print(const Reading& r) {
    const int abs_t = r.temp_c10 < 0 ? -r.temp_c10 : r.temp_c10;
    std::printf("T=%s%d.%d C  H=%u.%u %%\n", r.temp_c10 < 0 ? "-" : "", abs_t / 10, abs_t % 10,
                static_cast<unsigned>(r.humidity_p10 / 10), static_cast<unsigned>(r.humidity_p10 % 10));
}

// Tham số ra: hàm điền vào 'out', trả về thành công hay không.
[[nodiscard]] bool read_sensor(std::uint16_t raw, Reading& out) {
    if (raw == 0xFFFF) return false;                                        // cảm biến không phản hồi
    out.temp_c10 = static_cast<std::int16_t>(static_cast<int>(raw) / 4 - 400);  // công thức minh hoạ
    out.humidity_p10 = 500;
    return true;
}

int main() {
    int value = 10, other = 20;

    const int* p_to_const = &value;   // không sửa được *p, đổi được p
    p_to_const = &other;

    int* const const_ptr = &value;    // sửa được *p, không đổi được p
    *const_ptr = 11;

    const int* const both = &value;   // không sửa được gì
    std::printf("value=%d other=%d *p_to_const=%d *both=%d\n", value, other, *p_to_const, *both);

    Reading r{};
    if (read_sensor(1700, r)) print(r);
    if (read_sensor(200, r)) print(r);
    if (!read_sensor(0xFFFF, r)) std::puts("cam bien loi");
    return 0;
}
