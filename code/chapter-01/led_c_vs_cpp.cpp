// So sánh cách C và cách C++ để bật/tắt một chân GPIO.
// Trên MCU thật, kGpioOdr là địa chỉ thanh ghi; ở đây dùng biến giả để chạy trên PC.
#include <cstdint>
#include <cstdio>

static volatile std::uint32_t fake_odr = 0;

// ---- Cách C: macro, không có kiểu, không kiểm tra ----
#define ODR (fake_odr)
#define LED_PIN 5
#define LED_ON()  (ODR |=  (1u << LED_PIN))
#define LED_OFF() (ODR &= ~(1u << LED_PIN))

// ---- Cách C++: hằng có kiểu, hàm inline, template kiểm tra pin lúc biên dịch ----
template <unsigned Pin>
struct Gpio {
    static_assert(Pin < 32, "Pin phải nằm trong 0..31");
    static void set()   { fake_odr |=  (1u << Pin); }
    static void clear() { fake_odr &= ~(1u << Pin); }
};

int main() {
    LED_ON();
    std::printf("C   : ODR = 0x%08X\n", static_cast<unsigned>(fake_odr));
    LED_OFF();

    Gpio<5>::set();
    std::printf("C++ : ODR = 0x%08X\n", static_cast<unsigned>(fake_odr));
    Gpio<5>::clear();

    // Gpio<40>::set();  // bỏ comment dòng này: lỗi biên dịch, không phải lỗi lúc chạy
    return 0;
}
