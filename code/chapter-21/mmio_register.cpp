// Mo hinh mot khoi thanh ghi ngoai vi (peripheral) bang struct co truong 'volatile', anh xa vao mot
// vung nho gia lap (tren MCU that day se la mot dia chi co dinh, vd 0x40020000 cho GPIOA tren STM32).
#include <cstdint>
#include <cstdio>

struct GpioRegs {
    volatile std::uint32_t moder;   // che do chan: 00 input, 01 output, ...
    volatile std::uint32_t odr;     // gia tri ngo ra
    volatile std::uint32_t idr;     // gia tri ngo vao (chi doc, phan cung ghi)
};

// Tren MCU that: GpioRegs* const GPIOA = reinterpret_cast<GpioRegs*>(0x48000000);
// Tren host: gia lap bang mot vung nho tinh dong vai tro "thanh ghi phan cung".
static GpioRegs g_fake_gpio{};
static GpioRegs* const GPIOA = &g_fake_gpio;

constexpr std::uint32_t bit(unsigned n) { return 1u << n; }

void gpio_set_output(GpioRegs* gpio, unsigned pin) {
    gpio->moder |= bit(pin * 2);        // set bit thap cua cap 2 bit (che do output = 01)
    gpio->moder &= ~bit(pin * 2 + 1);   // xoa bit cao
}

void gpio_write(GpioRegs* gpio, unsigned pin, bool high) {
    if (high) gpio->odr |= bit(pin);
    else gpio->odr &= ~bit(pin);
}

int main() {
    gpio_set_output(GPIOA, 5);
    std::printf("MODER sau khi cau hinh chan 5 lam output = 0x%08X\n", static_cast<unsigned>(GPIOA->moder));

    gpio_write(GPIOA, 5, true);
    std::printf("ODR sau khi bat chan 5 = 0x%08X\n", static_cast<unsigned>(GPIOA->odr));

    gpio_write(GPIOA, 5, false);
    std::printf("ODR sau khi tat chan 5 = 0x%08X\n", static_cast<unsigned>(GPIOA->odr));

    // Gia lap phan cung ghi vao IDR (nhu the mot chan input vua doi trang thai).
    GPIOA->idr = bit(3);
    std::printf("IDR (phan cung bao chan 3 dang muc cao) = 0x%08X\n", static_cast<unsigned>(GPIOA->idr));
}
