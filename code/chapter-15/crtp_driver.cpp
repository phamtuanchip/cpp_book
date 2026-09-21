// CRTP (Curiously Recurring Template Pattern): "đa hình tĩnh" — chọn hàm lúc BIÊN DỊCH,
// không cần vtable, không cần con trỏ ảo. Đánh đổi: không đổi được loại lúc chạy.
#include <cstdint>
#include <cstdio>

// Lớp cơ sở nhận chính lớp con làm tham số template, rồi ép kiểu 'this' để gọi hàm của con.
template <typename Derived>
class DriverBase {
public:
    void init() { static_cast<Derived*>(this)->init_impl(); }
    void write(std::uint8_t value) { static_cast<Derived*>(this)->write_impl(value); }
};

class UartDriver : public DriverBase<UartDriver> {
public:
    void init_impl() { std::puts("UartDriver: init"); }
    void write_impl(std::uint8_t v) { std::printf("UartDriver: gui byte 0x%02X\n", static_cast<unsigned>(v)); }
};

class SpiDriver : public DriverBase<SpiDriver> {
public:
    void init_impl() { std::puts("SpiDriver: init"); }
    void write_impl(std::uint8_t v) { std::printf("SpiDriver: dich byte 0x%02X ra MOSI\n", static_cast<unsigned>(v)); }
};

// Hàm nhận driver bất kỳ qua template: gọi được init()/write() dù không có lớp cơ sở ảo nào,
// không có vtable, và trình biên dịch có thể inline thẳng vào init_impl/write_impl.
template <typename Driver>
void send_hello(Driver& drv) {
    drv.init();
    drv.write(0x48);   // 'H'
    drv.write(0x49);   // 'I'
}

int main() {
    UartDriver uart;
    SpiDriver spi;
    send_hello(uart);
    send_hello(spi);
    return 0;
}
