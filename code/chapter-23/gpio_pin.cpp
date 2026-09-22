// GPIO driver mong: moi chan la MOT KIEU rieng qua template (ket hop CRTP/template chuong 15 voi
// MMIO chuong 21). Khong vtable, khong con tro ham - trinh bien dich thuong inline het thanh vai lenh doc/ghi thanh ghi.
#include <cstdint>
#include <cstdio>

struct GpioRegs {
    volatile std::uint32_t moder;
    volatile std::uint32_t odr;
    volatile std::uint32_t idr;
};

static GpioRegs g_fake_porta{};
static GpioRegs g_fake_portb{};

enum class Mode : std::uint32_t { Input = 0b00, Output = 0b01 };

// Port va Pin la tham so template KHONG PHAI KIEU (gia tri hang so): moi to hop (Port, Pin) la mot
// kieu GpioPin<...> rieng biet, giai quyet hoan toan luc bien dich.
template <GpioRegs* Port, unsigned Pin>
class GpioPin {
public:
    static void set_mode(Mode m) {
        const auto val = static_cast<std::uint32_t>(m);
        Port->moder = (Port->moder & ~(0b11u << (Pin * 2))) | (val << (Pin * 2));
    }
    static void write(bool high) {
        if (high) Port->odr |= (1u << Pin);
        else Port->odr &= ~(1u << Pin);
    }
    static void toggle() { Port->odr ^= (1u << Pin); }
    static bool read() { return (Port->idr & (1u << Pin)) != 0; }
};

using LedPin = GpioPin<&g_fake_porta, 5>;
using ButtonPin = GpioPin<&g_fake_portb, 3>;

int main() {
    LedPin::set_mode(Mode::Output);
    ButtonPin::set_mode(Mode::Input);

    LedPin::write(true);
    std::printf("ODR porta sau khi bat LED = 0x%08X\n", static_cast<unsigned>(g_fake_porta.odr));

    for (int i = 0; i < 3; ++i) LedPin::toggle();
    std::printf("ODR porta sau 3 lan toggle (le -> tat) = 0x%08X\n", static_cast<unsigned>(g_fake_porta.odr));

    // Gia lap nut nhan duoc "an" (phan cung ghi vao IDR cua portb).
    g_fake_portb.idr |= (1u << 3);
    std::printf("ButtonPin::read() = %s\n", ButtonPin::read() ? "dang an" : "nha");

    // LedPin khong co truong du lieu nao (chi ham static) - la "kieu rong". Theo chuan C++,
    // sizeof toi thieu la 1 (de hai doi tuong khac nhau co dia chi khac nhau), du chuong trinh
    // nay khong bao gio tao doi tuong LedPin nao - chi goi ham static qua ten kieu.
    std::printf("sizeof(LedPin) = %zu (kieu rong; khong doi tuong nao duoc tao trong vi du nay)\n", sizeof(LedPin));
}
